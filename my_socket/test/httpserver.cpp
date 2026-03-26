#include "HttpServer.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
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
#include "TimeStamp.h"
#include "UserRepository.h"
#include "WarmupWorker.h"

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

const UserRepository &GetUserRepository() {
  static const UserRepository repository("127.0.0.1", 3306, "netdisk_app", "123456", "netdisk");
  return repository;
}

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

void Relocation(HttpResponse *response, const char *location = "/") {
  response->SetContentType("text/html; charset=UTF-8");
  response->SetStatusCode(HttpResponse::HttpStatusCode::K302K);
  response->SetStatusMessage("Moved Temporarily");
  response->AddHeader("Location", location);  // 重发url/
  response->SetClose();                       // 防止上一信息污染重定位
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

std::string BuildFileHtml(const std::string &dir) {
  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  std::string file;
  for (const auto &filename : files) {
    file += "<tr><td class=\"file-name\">";
    file += filename;
    file += "</td><td class=\"actions\">";
    file += "<a class=\"btn btn-view\" href=\"/open/";
    file += filename;
    file += "\">浏览</a>";
    file += "<a class=\"btn btn-download\" href=\"/download/";
    file += filename;
    file += "\">下载</a>";
    file += "<a class=\"btn btn-delete\" href=\"/delete/";
    file += filename;
    file += "\">删除</a></td></tr>\n";
  }

  std::string tmp = "<!--filelist-->";
  std::string body = HttpServer::ReadFile("../static/fileserver.html");
  body.replace(body.find(tmp), tmp.length(), file);
  return body;
}

std::string BuildAnomFileHtml(const std::string &dir) {
  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  std::string file;
  for (const auto &filename : files) {
    file += "<tr><td class=\"file-name\">";
    file += filename;
    file += "</td><td class=\"actions\">";
    file += "<a class=\"btn btn-view\" href=\"/open/";
    file += filename;
    file += "\">浏览</a></td></tr>\n";
  }

  std::string tmp = "<!--filelist-->";
  std::string body = HttpServer::ReadFile("../static/anomfileserver.html");
  body.replace(body.find(tmp), tmp.length(), file);
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

void Httpopen(const std::string &filename, HttpResponse *response) {
  // std::cout<<"open file "<<filename<<std::endl;
  if (IsFindInDir(filename, "../files/")) {  // 安全检查，防止路径穿越攻击
    std::string s = filename.substr(filename.find_last_of('.') + 1);
    if (s == "txt" || s == "pdf" || s == "doc" || s == "docx" || s == "jpg" || s == "png" || s == "html") {
      if (s == "txt") {
        response->SetContentType("text/plain; charset=UTF-8");
      }
      if (s == "pdf") {
        response->SetContentType("application/pdf");
      }
      if (s == "doc") {
        response->SetContentType("application/msword");
      }
      if (s == "docx") {
        response->SetContentType("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
      }
      if (s == "jpg") {
        response->SetContentType("image/jpeg");
      }
      if (s == "png") {
        response->SetContentType("image/png");
      }
      if (s == "html") {
        response->SetContentType("text/html; charset=UTF-8");
      }
      response->SetResponseBody(PageCacheService::Instance().GetOrBuild(
          "file:view:" + filename, 60, [filename]() { return HttpServer::ReadFile("../files/" + filename); }));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
      LOG_INFO << "Open file " << filename << " success!";
    } else {
      // std::cout<<"open file 1"<<filename<<std::endl;
      Errif(true, "Open file failed!");
      response->SetResponseBody("Open file ");
      Relocation(response);
    }
  } else {
    // std::cout<<"open file 2"<<filename<<std::endl;
    Errif(true, "Open file failed!");
    Relocation(response);
  }
}
void Httpdownload(const std::string &filename, HttpResponse *response) {
  if (IsFindInDir(filename, "../files/")) {  // 安全检查，防止路径穿越攻击
    int filefd = ::open(("../files/" + filename).c_str(), O_RDONLY);
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
      LOG_INFO << "Download file " << filename << " success!";
    }
  } else {
    Errif(true, "Download file failed!");
    OpenFileSystem(response);
  }
}
void Httpdelete(const std::string &filename, HttpResponse *response) {
  bool deleted = false;
  if (IsFindInDir(filename, "../files/")) {  // 安全检查，防止路径穿越攻击
    if (remove(("../files/" + filename).c_str()) != 0) {
      Errif(true, "Delete file failed!");
    } else {
      deleted = true;
      LOG_INFO << "Delete file " << filename << " success!";
    }
  } else {
    Errif(true, "Delete file failed!");
  }
  if (deleted) {
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
  if (request.GetMethodString() == "GET") {
    const std::string &url = request.GetURL();
    if (url == "/") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(PageCacheService::Instance().GetOrBuild(
          "page:home", 60, []() { return HttpServer::ReadFile("../static/index.html"); }));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
    } else if (url.substr(0, 5) == "/open") {  // 不包括\0 中文未解码
      std::string filename = url.substr(6);
      Httpopen(filename, response);  // 不带斜杠
    } else if (url.substr(0, 9) == "/download") {
      Httpdownload(url.substr(10), response);
    } else if (url.substr(0, 7) == "/delete") {
      Httpdelete(url.substr(8), response);
    } else if (url.substr(0, 5) == "/anom") {
      OpenGuestFileSystem(response);
    } else if (url.substr(0, 4) == "/ret") {
      Relocation(response);
    } else {
      response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
      response->SetStatusMessage("BAD_RESQUEST");
      response->SetClose();
    }
  } else if (request.GetMethodString() == "POST") {  // post返回的内容需要有body
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
        if (GetUserRepository().VerifyPlainPassword(username, password)) {
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
      PageCacheService::Instance().InvalidateFileListPages();
      OpenFileSystem(response);
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
  std::string ip = "0.0.0.0";
  int port = 80;
  if (argc == 3) {
    ip = argv[1];
    port = std::atoi(argv[2]);
  }
  async_log = std::make_unique<AsyncLogging>("");
  Logger::SetOutput(AsyncOutput);
  Logger::SetFlush(AsyncFlush);
  async_log->Start();
  auto httpserver = std::make_unique<HttpServer>(ip.c_str(), port);
  PageCacheService::Instance().GetOrBuild(
          "page:home", 60, []() { return HttpServer::ReadFile("../static/index.html"); });  // 预热首页缓存
  WarmupWorker::Instance().Start();
  if (argc == 2) {  // 加一个参数变echo_server
    httpserver->SetMessageCallBack(Message);
  } else {
    httpserver->SetHttpResponseCallBack(Http);
  }
  // httpserver->OnTimerEvery(1.0, LogMetrics);

  httpserver->Start();
  WarmupWorker::Instance().Stop();
  return 0;
}
