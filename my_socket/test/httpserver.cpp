#include "HttpServer.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "AsyncLogging.h"
#include "Connection.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "PageCacheService.h"
// #include "Metrics.h"
#include "Config.h"
#include "FileMetaRepository.h"
#include "RedisClient.h"
#include "SessionManager.h"
#include "TimeStamp.h"
#include "UserRepository.h"
#include "WarmupWorker.h"

void Relocation(HttpResponse *response, const char *location = "/") {
  response->SetContentType("text/html; charset=UTF-8");
  response->SetStatusCode(HttpResponse::HttpStatusCode::K302K);
  response->SetStatusMessage("Moved Temporarily");
  response->AddHeader("Location", location);  // 重发url/
  response->SetClose();                       // 防止上一信息污染重定位
}

namespace {
constexpr int kSessionTtlSeconds = 30 * 60;
constexpr int kGuestSessionTtlSeconds = 24 * 60 * 60;
constexpr char kSessionCookieName[] = "my_socket_sid";
constexpr char kGuestCookieName[] = "guest_sid";
constexpr char kGuestSessionPrefix[] = "guest:session:";
constexpr char kTotalRequestCounterKey[] = "metrics:requests:total";
constexpr char kInstanceRequestCounterPrefix[] = "metrics:requests:instance:";
constexpr char kRateLimitPrefix[] = "rate_limit:";
constexpr int kGuestRecentViewKeep = 10;
constexpr int kFileListLimit = 200;  // 最多文件量

std::string g_instance_name = "default";
int g_guest_browse_rate_limit_per_min = 120;
std::unique_ptr<SessionManager> g_session_manager;

struct RequestContext {
  bool is_guest = true;
  int64_t user_id = -1;
  std::string username;
  std::string guest_id;
};

std::string TrimAsciiSpaces(const std::string &text) {  // 去除字符串前后的空白字符
  size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }

  size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return text.substr(begin, end - begin);
}

std::string GenerateGuestSessionId() {
  static thread_local std::mt19937_64 rng(std::random_device {}());
  std::uniform_int_distribution<unsigned int> dist(0, 255);

  std::string token = "guest_";
  for (int i = 0; i < 16; ++i) {
    const unsigned int byte = dist(rng);
    const char hex[] = "0123456789abcdef";
    token.push_back(hex[(byte >> 4U) & 0x0fU]);
    token.push_back(hex[byte & 0x0fU]);  // 每个随机字节转换为两个十六进制字符，增加token的长度和复杂度
  }
  return token;
}

std::string BuildInstanceRequestCounterKey() { return std::string(kInstanceRequestCounterPrefix) + g_instance_name; }

void SetJsonResponse(HttpResponse *response, const std::string &body) {
  response->SetContentType("application/json; charset=UTF-8");
  response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
  response->SetStatusMessage("OK");
  response->SetResponseBody(std::string(body));
}
void SetJsonError(HttpResponse *response, HttpResponse::HttpStatusCode code, const std::string &status,
                  const std::string &error_message) {
  response->SetStatusCode(code);
  response->SetStatusMessage(std::string(status));
  response->SetContentType("application/json; charset=UTF-8");
  response->SetResponseBody(std::string("{\"error\":\"") + error_message + "\"}");
}
// void AddRuntimeHeaders(HttpResponse *response) {
//   response->AddHeader(std::string("X-Instance-Name"), std::string(g_instance_name));
//   response->AddHeader(std::string("X-Process-Id"), std::to_string(::getpid()));
// }

void TrackRequest() {  // 统计请求总数和每个实例的请求数，使用Redis原子递增命令，避免高并发下的竞争问题
  RedisClient::Instance().IncrBy(kTotalRequestCounterKey, 1);
  RedisClient::Instance().IncrBy(BuildInstanceRequestCounterKey(), 1);
}

bool ParseInt64(const std::string &text, int64_t *value) {
  if (value == nullptr || text.empty()) {
    return false;
  }
  try {
    *value = std::stoll(text);
  } catch (...) {
    return false;
  }
  return true;
}

bool ExtractCookieValue(
    const std::string &cookie_header, const std::string &key,
    std::string *value) {  // 从Cookie头中提取指定键的值，解析Cookie字符串，处理多个Cookie项，返回找到的值
  if (value == nullptr || cookie_header.empty()) {
    return false;
  }

  size_t begin = 0;
  while (begin < cookie_header.size()) {
    size_t end = cookie_header.find(';', begin);
    if (end == std::string::npos) {
      end = cookie_header.size();
    }
    const std::string item = TrimAsciiSpaces(cookie_header.substr(begin, end - begin));
    const size_t equal = item.find('=');
    if (equal != std::string::npos) {
      const std::string name = TrimAsciiSpaces(item.substr(0, equal));
      if (name == key) {
        *value = item.substr(equal + 1);
        return true;
      }
    }
    begin = end + 1;
  }
  return false;
}
// token对应用户信息
bool CreateUserSession(const UserRecord &user, HttpResponse *response) {
  if (response == nullptr || g_session_manager == nullptr) {
    return false;
  }
  const std::string token = g_session_manager->CreateUserSession(user.id, user.username);
  if (token.empty()) {
    return false;
  }
  response->AddHeader(std::string("Set-Cookie"), std::string(kSessionCookieName) + "=" + token + "; Path=/; HttpOnly");
  return true;
}

void WriteWhoAmI(HttpResponse *response) {
  SetJsonResponse(response, "{\"instance\":\"" + g_instance_name + "\",\"pid\":" + std::to_string(::getpid()) + "}");
}

std::string BuildGuestSessionKey(const std::string &guest_id) { return std::string(kGuestSessionPrefix) + guest_id; }

void KeepGuestSessionAlive(const std::string &guest_id) {  // 访客会话保持
  if (guest_id.empty()) {
    return;
  }
  const std::string key = BuildGuestSessionKey(guest_id);
  std::string payload;
  const RedisClient::GetResult get_result = RedisClient::Instance().GetWithStatus(key, &payload);
  if (get_result == RedisClient::GetResult::kHit) {
    RedisClient::Instance().Expire(key, kGuestSessionTtlSeconds);
    return;
  }
  if (get_result == RedisClient::GetResult::kMiss) {
    RedisClient::Instance().SetEx(key, kGuestSessionTtlSeconds, "");
  }
}

void ParseSession(const HttpRequest &request, HttpResponse *response, RequestContext *ctx) {
  if (ctx == nullptr || response == nullptr) {
    return;
  }

  std::string token = request.GetHeader("X-Session-Token");
  if (token.empty()) {  // 获取会话令牌
    ExtractCookieValue(request.GetHeader("Cookie"), kSessionCookieName, &token);
  }

  if (!token.empty() && g_session_manager != nullptr) {
    UserSession session;
    if (g_session_manager->GetUserSession(token, &session)) {
      ctx->is_guest = false;
      ctx->user_id = session.user_id;
      ctx->username = session.username;
      g_session_manager->RefreshSession(token, kSessionTtlSeconds);
      return;
    }
  }

  std::string guest_id;
  ExtractCookieValue(request.GetHeader("Cookie"), kGuestCookieName, &guest_id);
  if (guest_id.empty()) {
    guest_id = GenerateGuestSessionId();
    response->AddHeader(std::string("Set-Cookie"),
                        std::string(kGuestCookieName) + "=" + guest_id + "; Path=/; HttpOnly");
  }
  ctx->guest_id = guest_id;
  KeepGuestSessionAlive(guest_id);
}

std::string BuildVisitCounterKeyByDate() {
  const std::time_t now = std::time(nullptr);
  std::tm local_tm{};
  localtime_r(&now, &local_tm);
  char buf[32] = {0};
  std::strftime(buf, sizeof(buf), "visit:%Y%m%d", &local_tm);
  return std::string(buf);
}

void TrackBrowseVisit() { RedisClient::Instance().IncrBy(BuildVisitCounterKeyByDate(), 1); }  // 每日浏览量

std::vector<std::string> SplitCommaList(const std::string &text) {
  std::vector<std::string> items;
  size_t begin = 0;
  while (begin < text.size()) {
    size_t end = text.find(',', begin);
    if (end == std::string::npos) {
      end = text.size();
    }
    std::string item = text.substr(begin, end - begin);
    if (!item.empty()) {
      items.push_back(item);
    }
    begin = end + 1;
  }
  return items;
}

std::string JoinCommaList(const std::vector<std::string> &items) {
  std::string text;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i > 0) {
      text += ",";
    }
    text += items[i];
  }
  return text;
}

void RecordGuestRecentView(const RequestContext &ctx, const std::string &file_ref) {
  if (!ctx.is_guest || ctx.guest_id.empty() || file_ref.empty()) {
    return;
  }

  const std::string key = BuildGuestSessionKey(ctx.guest_id);
  std::string current;
  std::vector<std::string> views;
  if (RedisClient::Instance().Get(key, &current)) {
    views = SplitCommaList(current);
  }

  views.erase(std::remove(views.begin(), views.end(), file_ref),
              views.end());  // 移除已存在的记录，保持最近浏览记录的唯一性和顺序
  views.insert(views.begin(), file_ref);
  if (views.size() > static_cast<size_t>(kGuestRecentViewKeep)) {
    views.resize(kGuestRecentViewKeep);
  }
  RedisClient::Instance().SetEx(key, kGuestSessionTtlSeconds, JoinCommaList(views));
}

bool CheckRateLimit(const std::string &api_name, const std::string &uid, int limit_per_min, HttpResponse *response) {
  if (api_name.empty() || uid.empty() || limit_per_min <= 0) {
    return true;
  }

  const std::string key = std::string(kRateLimitPrefix) + api_name + ":" + uid;
  int64_t current = 0;  // 目前的单用户请求计数
  if (!RedisClient::Instance().IncrByWithValue(key, 1, &current)) {
    return true;  // Redis 失败时放行，避免缓存/限流组件拖垮主流程。
  }
  RedisClient::Instance().Expire(key, 60);
  if (current <= limit_per_min) {
    return true;
  }

  response->SetStatusCode(HttpResponse::HttpStatusCode::K429TOOMANYREQUESTS);
  response->SetStatusMessage("TOO_MANY_REQUESTS");
  response->SetContentType("application/json; charset=UTF-8");
  response->SetResponseBody(std::string("{\"error\":\"too_many_requests\"}"));
  return false;
}

std::string GetRateLimitSubject(const RequestContext &ctx) {
  if (!ctx.guest_id.empty()) {
    return ctx.guest_id;
  }
  return "guest_unknown";
}

bool CheckGuestRateLimit(const std::string &api_name, const RequestContext &ctx, HttpResponse *response) {
  if (!ctx.is_guest) {
    return true;
  }
  return CheckRateLimit(api_name, GetRateLimitSubject(ctx), g_guest_browse_rate_limit_per_min, response);
}
}  // namespace

// std::string GetEnvOrDefault(const char *name, const char *default_value) { // 合法检查
//   const char *value = std::getenv(name);
//   if (value == nullptr || value[0] == '\0') {
//     return default_value;
//   }
//   return value;
// }

// int GetEnvPortOrDefault(const char *name, int default_port) {
//   const char *value = std::getenv(name);
//   if (value == nullptr || value[0] == '\0') {
//     return default_port;
//   }

//   char *end = nullptr;
//   long parsed = std::strtol(value, &end, 10);
//   if (end == value || *end != '\0' || parsed <= 0 || parsed > 65535) {
//     return default_port;
//   }
//   return static_cast<int>(parsed);
// }

bool ParseFormField(const std::string &body, const std::string &key, std::string *value) {  // 按字段查找
  const std::string field = key + "=";
  const size_t begin = body.find(field);
  if (begin == std::string::npos) {
    return false;
  }

  const size_t value_begin = begin + field.size();
  size_t value_end = body.find('&', value_begin);
  if (value_end == std::string::npos) {
    value_end = body.size();
  }
  *value = body.substr(value_begin, value_end - value_begin);
  return true;
}

void FindAllFiles(const std::string &dir, std::vector<std::string> *files) {
  // int count = 0;
  DIR *dp = nullptr;
  struct dirent *entry = nullptr;
  if ((dp = opendir(dir.c_str())) == nullptr) {
    Errif(true, "Open directory failed!");
    return;
  }
  while ((entry = readdir(dp)) != nullptr) {
    std::string filename = entry->d_name;
    if (filename != "." && filename != "..") {  // string在前会调用重载
      files->push_back(filename);
      // count++;
    }
  }
  // LOG_INFO << "Find " << count << " files in directory " << dir;
  closedir(dp);
}

std::vector<FileMeta> ListLatestFileMetaForPage() {
  try {
    return FileMetaRepository::Instance().ListLatestFiles(kFileListLimit);
  } catch (const std::exception &e) {
    LOG_ERROR << "ListLatestFiles failed: " << e.what();
    return {};
  }
}

bool IsFindInDir(const std::string &file, const std::string &dir) {
  DIR *dp = nullptr;
  struct dirent *entry = nullptr;
  if ((dp = opendir(dir.c_str())) == nullptr) {
    Errif(true, "Open directory failed!");
    return false;
  }
  while ((entry = readdir(dp)) != nullptr) {
    if (file == entry->d_name) {  // 字符串比较
      closedir(dp);
      return true;
    }
  }
  closedir(dp);
  return false;
}

std::string BuildUserFileRows(const std::string &dir) {
  std::string rows;
  const std::vector<FileMeta> metas = ListLatestFileMetaForPage();
  if (!metas.empty()) {
    for (const auto &meta : metas) {
      rows += "<tr><td class=\"file-name\">";
      rows += meta.filename;
      rows += "</td><td class=\"actions\">";
      const std::string id_suffix =
          "?id=" +
          std::to_string(meta.id); // 嵌入文件ID作为查询参数
      rows += "<a class=\"btn btn-view\" href=\"/open/";
      rows += meta.filename;
      rows += id_suffix;
      rows += "\">浏览</a>";
      rows += "<a class=\"btn btn-download\" href=\"/download/";
      rows += meta.filename;
      rows += id_suffix;
      rows += "\">下载</a>";
      rows += "<a class=\"btn btn-delete\" href=\"/delete/";
      rows += meta.filename;
      rows += id_suffix;
      rows += "\">删除</a></td></tr>\n";
    }
    return rows;
  }
  // 降级
  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  for (const auto &filename : files) {
    rows += "<tr><td class=\"file-name\">";
    rows += filename;
    rows += "</td><td class=\"actions\">";
    rows += "<a class=\"btn btn-view\" href=\"/open/";
    rows += filename;
    rows += "\">浏览</a>";
    rows += "<a class=\"btn btn-download\" href=\"/download/";
    rows += filename;
    rows += "\">下载</a>";
    rows += "<a class=\"btn btn-delete\" href=\"/delete/";
    rows += filename;
    rows += "\">删除</a></td></tr>\n";
  }
  return rows;
}

std::string BuildGuestFileRows(const std::string &dir) {
  std::string rows;
  const std::vector<FileMeta> metas = ListLatestFileMetaForPage();
  if (!metas.empty()) {
    for (const auto &meta : metas) {
      rows += "<tr><td class=\"file-name\">";
      rows += meta.filename;
      rows += "</td><td class=\"actions\">";
      rows += "<a class=\"btn btn-view\" href=\"/open/";
      rows += meta.filename;
      rows += "?id=" + std::to_string(meta.id);
      rows += "\">浏览</a></td></tr>\n";
    }
    return rows;
  }

  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  for (const auto &filename : files) {
    rows += "<tr><td class=\"file-name\">";
    rows += filename;
    rows += "</td><td class=\"actions\">";
    rows += "<a class=\"btn btn-view\" href=\"/open/";
    rows += filename;
    rows += "\">浏览</a></td></tr>\n";
  }
  return rows;
}

std::string BuildFileHtml(const std::string &dir) {
  const std::string file = BuildUserFileRows(dir);

  std::string tmp = "<!--filelist-->";
  std::string body = HttpServer::ReadFile("../static/fileserver.html");
  const size_t pos = body.find(tmp);
  if (pos != std::string::npos) {
    body.replace(pos, tmp.length(), file);
  }
  return body;
}

std::string BuildAnomFileHtml(const std::string &dir) {
  const std::string file = BuildGuestFileRows(dir);

  std::string tmp = "<!--filelist-->";
  std::string body = HttpServer::ReadFile("../static/anomfileserver.html");
  const size_t pos = body.find(tmp);
  if (pos != std::string::npos) {
    body.replace(pos, tmp.length(), file);
  }
  return body;
}

void OpenFileSystem(HttpResponse *response) {  // 不设置域名防止别人直接闯入
  response->SetContentType("text/html; charset=UTF-8");
  // response->SetResponseBody(BuildFileHtml("../files"));
  response->SetResponseBody(
      PageCacheService::Instance().GetOrBuild("page:filelist:user", 60, []() { return BuildFileHtml("../files"); }));
  response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
  response->SetStatusMessage("OK");
}

void OpenGuestFileSystem(HttpResponse *response) {
  response->SetContentType("text/html; charset=UTF-8");
  response->SetResponseBody(PageCacheService::Instance().GetOrBuild("page:filelist:guest", 60,
                                                                    []() { return BuildAnomFileHtml("../files"); }));
  response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
  response->SetStatusMessage("OK");
}

std::string DetectFileType(const std::string &filename) {
  const size_t dot = filename.find_last_of('.');
  if (dot == std::string::npos || dot + 1 >= filename.size()) {
    return "unknown";
  }
  return filename.substr(dot + 1);
}

std::string GetPathBasename(const std::string &path) {
  if (path.empty()) {
    return "";
  }
  const size_t slash = path.find_last_of('/');
  if (slash == std::string::npos || slash + 1 >= path.size()) {
    return path;
  }
  return path.substr(slash + 1);
}

void SetContentTypeByExt(const std::string &ext, HttpResponse *response) {
  if (ext == "txt") {
    response->SetContentType("text/plain; charset=UTF-8");
  } else if (ext == "pdf") {
    response->SetContentType("application/pdf");
  } else if (ext == "doc") {
    response->SetContentType("application/msword");
  } else if (ext == "docx") {
    response->SetContentType("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
  } else if (ext == "jpg") {
    response->SetContentType("image/jpeg");
  } else if (ext == "png") {
    response->SetContentType("image/png");
  } else if (ext == "html") {
    response->SetContentType("text/html; charset=UTF-8");
  }
}

void Httpopen(const std::string &filename, const std::string &filepath, HttpResponse *response) {
  const std::string target_path = filepath.empty() ? ("../files/" + filename) : filepath;

  const std::string filename_for_type = !filename.empty() ? GetPathBasename(filename) : GetPathBasename(target_path);
  const std::string filetype = DetectFileType(filename_for_type);
  if (filetype != "txt" && filetype != "pdf" && filetype != "doc" && filetype != "docx" && filetype != "jpg" &&
      filetype != "png" && filetype != "html") {
    Errif(true, "Open file failed!");
    response->SetResponseBody("Open file ");
    Relocation(response);
    return;
  }

  SetContentTypeByExt(filetype, response);
  response->SetResponseBody(PageCacheService::Instance().GetOrBuild(
      "file:view:" + target_path, 60, [target_path]() { return HttpServer::ReadFile(target_path); }));
  response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
  response->SetStatusMessage("OK");
  LOG_INFO << "Open file path=" << target_path << " success!";
}

void Httpdownload(const std::string &filename, const std::string &filepath, HttpResponse *response) {
  const std::string target_path = filepath.empty() ? ("../files/" + filename) : filepath;

  int filefd = ::open(target_path.c_str(), O_RDONLY);
  if (filefd < 0) {
    Errif(true, "Download file failed!");
    OpenFileSystem(response);
  } else {
    struct stat file_stat = {};
    fstat(filefd, &file_stat);
    response->SetContentLength(static_cast<int>(file_stat.st_size));
    response->SetContentType("application/octet-stream");
    response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
    response->SetBodyType("FILE_TYPE");
    response->SetFileId(filefd);
    LOG_INFO << "Download file path=" << target_path << " success!";
  }
}

bool ExtractUploadFilename(const HttpRequest &request, std::string *filename) {
  if (filename == nullptr) {
    return false;
  }
  size_t fn_index = request.GetBody().find("filename");
  if (fn_index == std::string::npos) {
    return false;
  }
  fn_index += std::string("filename=\"").size();
  size_t fn_end_index = request.GetBody().find("\"\r\n", fn_index);
  if (fn_end_index == std::string::npos || fn_end_index <= fn_index) {
    return false;
  }
  *filename = request.GetBody().substr(fn_index, fn_end_index - fn_index);
  return !filename->empty();
}

int64_t GetFileSize(const std::string &filepath) {
  struct stat file_stat {};
  if (stat(filepath.c_str(), &file_stat) != 0) {
    return 0;
  }
  return static_cast<int64_t>(file_stat.st_size);
}

void PersistUploadedFileMeta(const RequestContext &ctx, const std::string &filename) {
  if (ctx.is_guest || ctx.user_id <= 0 || filename.empty()) {
    return;
  }
  const std::string filepath = "../files/" + filename;
  // int64_t file_id = 0;  // 外键
  try {
    FileMetaRepository::Instance().CreateFileMeta(ctx.user_id, filename, filepath, GetFileSize(filepath),
                                                  DetectFileType(filename), "" /*, &file_id*/);
  } catch (const std::exception &e) {
    LOG_ERROR << "CreateFileMeta failed: " << e.what();
  }
}

bool ResolveFilenameByMetaId(int64_t file_id, std::string *filename, std::string *filepath, HttpResponse *response) {
  if (response == nullptr || filename == nullptr || filepath == nullptr || file_id <= 0) {
    return false;
  }
  FileMeta meta;
  try {
    if (!FileMetaRepository::Instance().GetFileMetaById(file_id, &meta)) {
      SetJsonError(response, HttpResponse::HttpStatusCode::K404NOTFOUND, "NOT_FOUND", "file_meta_not_found");
      return false;
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "ResolveFilenameByMetaId failed: " << e.what();
    SetJsonError(response, HttpResponse::HttpStatusCode::K500INTERNALSERVERERROR, "INTERNAL_SERVER_ERROR",
                 "file_meta_query_failed");
    return false;
  }
  *filename = meta.filename;
  *filepath = meta.filepath;
  if (filepath->empty()) {
    *filepath = "../files/" + GetPathBasename(*filename);
  }
  if (filename->empty()) {
    *filename = GetPathBasename(*filepath);
  }
  return !filename->empty() && !filepath->empty();
}

void Httpdelete(const std::string &filename, int64_t file_id, const RequestContext &ctx, HttpResponse *response) {
  if (ctx.is_guest || ctx.user_id <= 0) {
    SetJsonError(response, HttpResponse::HttpStatusCode::K403FORBIDDEN, "FORBIDDEN", "login_required");
    return;
  }

  FileMeta meta;
  try {
    bool found = false;
    if (file_id > 0) {
      found = FileMetaRepository::Instance().GetFileMetaById(file_id, &meta);
    } else {
      found = FileMetaRepository::Instance().GetFileMetaByFilename(filename, &meta);
    }
    if (!found) {
      SetJsonError(response, HttpResponse::HttpStatusCode::K404NOTFOUND, "NOT_FOUND", "file_meta_not_found");
      return;
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "GetFileMetaByFilename failed: " << e.what();
    SetJsonError(response, HttpResponse::HttpStatusCode::K500INTERNALSERVERERROR, "INTERNAL_SERVER_ERROR",
                 "file_meta_query_failed");
    return;
  }
  if (meta.user_id != ctx.user_id) {
    SetJsonError(response, HttpResponse::HttpStatusCode::K403FORBIDDEN, "FORBIDDEN", "no_permission_delete");
    return;
  }

  std::string target_filename = meta.filename.empty() ? filename : meta.filename;
  std::string target_filepath = meta.filepath;
  if (target_filepath.empty()) {
    target_filepath = "../files/" + GetPathBasename(target_filename);
  }
  if (target_filename.empty()) {
    target_filename = GetPathBasename(target_filepath);
  }
  if (target_filename.empty() || target_filepath.empty()) {
    SetJsonError(response, HttpResponse::HttpStatusCode::K404NOTFOUND, "NOT_FOUND", "file_not_found");
    return;
  }

  bool deleted = false;
  if (remove(target_filepath.c_str()) != 0) {
    Errif(true, "Delete file failed!");
  } else {
    deleted = true;
    LOG_INFO << "Delete file path=" << target_filepath << " success!";
  }
  if (deleted) {
    try {
      FileMetaRepository::Instance().DeleteFileMetaById(meta.id);
    } catch (const std::exception &e) {
      LOG_ERROR << "DeleteFileMetaById failed: " << e.what();
    }
    PageCacheService::Instance().InvalidateFileListPages();
  }
  OpenFileSystem(response);
}

void Message(const std::shared_ptr<Connection> &conn) {  // 注册回调函数,需要修改内部元素所以不能设为const
  // conn->Read();
  LOG_INFO << "New message from client " << conn->GetFd() << " : " << conn->ReadInputBuffer();
  conn->Send(conn->RetriveInputBuffer());
}

void Http(const HttpRequest &request, HttpResponse *response) {
  // AddRuntimeHeaders(response);
  TrackRequest();

  const std::string method = request.GetMethodString();
  const std::string url = request.GetURL();
  if (method == "GET" && url == "/whoami") {
    WriteWhoAmI(response);
    return;
  }

  RequestContext ctx;  // 请求上下文，包含用户信息和状态，在处理请求的过程中传递，避免重复解析和查询
  ParseSession(request, response, &ctx);

  if (method == "GET") {
    if (url == "/") {
      if (!ctx.is_guest) {  // 登录用户直接进入文件系统，访客进入欢迎页
        OpenFileSystem(response);
        return;
      }
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(PageCacheService::Instance().GetOrBuild(
          "page:home", 60, []() { return HttpServer::ReadFile("../static/index.html"); }));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
    } else if (url == "/files") {
      if (ctx.is_guest) {
        OpenGuestFileSystem(response);
      } else {
        OpenFileSystem(response);
      }
    } else if (url.substr(0, 5) == "/open") {  // 不包括\0 中文未解码
      std::string filename = url.substr(6);
      std::string filepath;
      int64_t file_id = 0;
      const std::string file_id_text = request.GetParams("id");
      if (!file_id_text.empty()) {
        if (!ParseInt64(file_id_text, &file_id) || file_id <= 0) {
          SetJsonError(response, HttpResponse::HttpStatusCode::K400BADREQUEST, "BAD_REQUEST", "invalid_file_id");
          return;
        }
        if (!ResolveFilenameByMetaId(file_id, &filename, &filepath, response)) {
          return;
        }
      }
      if (!CheckGuestRateLimit("/open", ctx, response)) {
        return;
      }
      Httpopen(filename, filepath, response);  // 不带斜杠
      TrackBrowseVisit();
      if (ctx.is_guest) {
        if (file_id > 0) {
          RecordGuestRecentView(ctx, "id:" + std::to_string(file_id));
        } else {
          RecordGuestRecentView(ctx, "name:" + filename);
        }
      }
    } else if (url.substr(0, 9) == "/download") {
      std::string filename = url.substr(10);
      std::string filepath;
      int64_t file_id = 0;
      const std::string file_id_text = request.GetParams("id");
      if (!file_id_text.empty()) {
        if (!ParseInt64(file_id_text, &file_id) || file_id <= 0) {
          SetJsonError(response, HttpResponse::HttpStatusCode::K400BADREQUEST, "BAD_REQUEST", "invalid_file_id");
          return;
        }
        if (!ResolveFilenameByMetaId(file_id, &filename, &filepath, response)) {
          return;
        }
      }
      if (!CheckGuestRateLimit("/download", ctx, response)) {
        return;
      }
      Httpdownload(filename, filepath, response);
      TrackBrowseVisit();
      if (ctx.is_guest) {
        if (file_id > 0) {
          RecordGuestRecentView(ctx, "id:" + std::to_string(file_id));
        } else {
          RecordGuestRecentView(ctx, "name:" + filename);
        }
      }
    } else if (url.substr(0, 7) == "/delete") {
      int64_t file_id = 0;
      const std::string file_id_text = request.GetParams("id");
      if (!file_id_text.empty() && (!ParseInt64(file_id_text, &file_id) || file_id <= 0)) {
        SetJsonError(response, HttpResponse::HttpStatusCode::K400BADREQUEST, "BAD_REQUEST", "invalid_file_id");
        return;
      }
      Httpdelete(url.substr(8), file_id, ctx, response);
    } else if (url.substr(0, 5) == "/anom") {
      OpenGuestFileSystem(response);
    } else if (url.substr(0, 4) == "/ret") {
      Relocation(response);
    } else {
      response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
      response->SetStatusMessage("BAD_RESQUEST");
      response->SetClose();
    }
  } else if (method == "POST") {  // post返回的内容需要有body
    // if (request.GetURL() == "/rpc") {
    //   HttpServer::HandleRpcRequest(request, response);
    // } else
    if (request.GetURL() == "/login") {
      const std::string &body = request.GetBody();
      std::string username;
      std::string password;
      if (!ParseFormField(body, "username", &username) || !ParseFormField(body, "password", &password)) {
        LOG_WARN << "Login request body parse failed";
        Relocation(response);
        return;
      }

      LOG_INFO << "New login request user=" << username;
      try {
        UserRecord user;
        if (UserRepository::Instance().VerifyPassword(username, password, &user)) {
          if (!CreateUserSession(user, response)) {
            LOG_ERROR << "Create login session failed, user=" << username;
            SetJsonError(response, HttpResponse::HttpStatusCode::K500INTERNALSERVERERROR, "INTERNAL_SERVER_ERROR",
                         "session_create_failed");
            return;
          }
          LOG_INFO << username << " login success!";
          OpenFileSystem(response);
        } else {
          LOG_INFO << username << " login failed!";
          Relocation(response);
        }
      } catch (const std::exception &e) {
        LOG_ERROR << "MySQL login verify failed, user=" << username << ", error=" << e.what();
        Relocation(response);
      }
    } else if (request.GetURL() == "/upload") {  // 上传成功才会到这里
      if (ctx.is_guest || ctx.user_id <= 0) {
        SetJsonError(response, HttpResponse::HttpStatusCode::K403FORBIDDEN, "FORBIDDEN", "login_required");
        return;
      }
      std::string uploaded_filename;
      const bool filename_ok = ExtractUploadFilename(request, &uploaded_filename);
      HttpServer::FileUpload(&request);
      if (filename_ok) {
        PersistUploadedFileMeta(ctx, uploaded_filename);
      }
      PageCacheService::Instance().InvalidateFileListPages();
      OpenFileSystem(response);
    } else if (request.GetURL() == "/logout") {
      std::string token = request.GetHeader("X-Session-Token");
      if (token.empty()) {
        ExtractCookieValue(request.GetHeader("Cookie"), kSessionCookieName, &token);
      }
      if (!token.empty() && g_session_manager != nullptr) {
        g_session_manager->DeleteSession(token);
      }
      response->AddHeader(std::string("Set-Cookie"),
                          std::string(kSessionCookieName) + "=deleted; Path=/; HttpOnly; Max-Age=0");
      Relocation(response, "/");
    } else {
      Errif(true, "Post request failed!");
      response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
      response->SetStatusMessage("BAD_RESQUEST");
    }
  } else {
    response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
    response->SetStatusMessage("BAD_RESQUEST");
    response->SetClose();
  }
}

void Every() { std::cout << TimeStamp::Now().ToFormattedString() << std::endl; }
// void LogMetrics() { Metrics::LogSnapshot(); }
std::unique_ptr<AsyncLogging> async_log;
void AsyncOutput(const char *msg, int len) { async_log->Append(msg, len); }
void AsyncFlush() { async_log->Flush(); }
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <config.yaml> [echo]" << std::endl;
    return 1;
  }

  AppConfig cfg;
  try {
    cfg = LoadConfigFromYaml(argv[1]);
  } catch (const std::exception &e) {
    std::cerr << "load config failed: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "start server:"
            << " host=" << cfg.server.host << " port=" << cfg.server.port << " instance=" << cfg.server.instance_name
            << " log_dir=" << cfg.log.dir << std::endl;
  g_instance_name = cfg.server.instance_name;
  g_guest_browse_rate_limit_per_min = cfg.feature.guest_browse_rate_limit_per_min;

  RedisClient::Init(cfg.redis.host, cfg.redis.port, cfg.redis.password, cfg.redis.db, cfg.redis.connect_timeout_ms,
                    cfg.redis.command_timeout_ms);
  UserRepository::Init(cfg.mysql.host, cfg.mysql.port, cfg.mysql.user, cfg.mysql.password, cfg.mysql.database,
                       cfg.mysql.connect_timeout_sec, cfg.mysql.read_timeout_sec, cfg.mysql.write_timeout_sec,
                       cfg.mysql.query_timeout_sec);
  FileMetaRepository::Init(cfg.mysql.host, cfg.mysql.port, cfg.mysql.user, cfg.mysql.password, cfg.mysql.database,
                           cfg.mysql.connect_timeout_sec, cfg.mysql.read_timeout_sec, cfg.mysql.write_timeout_sec,
                           cfg.mysql.query_timeout_sec);
  g_session_manager = std::make_unique<SessionManager>(kSessionTtlSeconds);

  async_log = std::make_unique<AsyncLogging>(cfg.log.dir.c_str());  // 存储路径
  Logger::SetOutput(AsyncOutput);
  Logger::SetFlush(AsyncFlush);
  async_log->Start();
  auto httpserver = std::make_unique<HttpServer>(cfg.server.host.c_str(), cfg.server.port, cfg.server.instance_name);
  PageCacheService::Instance().GetOrBuild(
      "page:home", 60, []() { return HttpServer::ReadFile("../static/index.html"); });  // 预热首页缓存
  WarmupWorker::Instance().Start();
  if (argc >= 3 && std::string(argv[2]) == "echo") {
    httpserver->SetMessageCallBack(Message);
  } else {
    httpserver->SetHttpResponseCallBack(Http);
  }
  // httpserver->OnTimerEvery(1.0, LogMetrics);
  httpserver->SetInstanceName(cfg.server.instance_name);
  httpserver->SetRpcTimeoutMs(cfg.feature.rpc_timeout_ms);
  httpserver->Start();
  WarmupWorker::Instance().Stop();
  return 0;
}
