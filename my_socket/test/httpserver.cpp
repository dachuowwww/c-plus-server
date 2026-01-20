#include "HttpServer.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "AsyncLogging.h"
#include "Connection.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "TimeStamp.h"

void Relocation(HttpResponse *response, const char *location = "/") {
  response->SetContentType("text/html; charset=UTF-8");
  response->SetStatusCode(HttpResponse::HttpStatusCode::K302K);
  response->SetStatusMessage("Moved Temporarily");
  response->AddHeader("Location", location);  // 重发url/
  response->SetClose();                       // 防止上一信息污染重定位
}
void FindAllFiles(const std::string &dir, std::vector<std::string> *files) {
  int count = 0;
  DIR *dp = nullptr;
  struct dirent *entry = nullptr;
  if ((dp = opendir(dir.c_str())) == nullptr) {
    LOG_ERROR << "Open directory " << dir << " failed!";
    return;
  }
  while ((entry = readdir(dp)) != nullptr) {
    std::string filename = entry->d_name;
    if (filename != "." && filename != "..") {  // string在前会调用重载
      files->push_back(filename);
      count++;
    }
  }
  LOG_INFO << "Find " << count << " files in directory " << dir;
  closedir(dp);
}

bool IsFindInDir(const std::string &file, const std::string &dir) {
  DIR *dp = nullptr;
  struct dirent *entry = nullptr;
  if ((dp = opendir(dir.c_str())) == nullptr) {
    LOG_ERROR << "Open directory " << dir << " failed!";
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
std::string ReadFile(const std::string &filename) {
  std::ifstream is(filename.c_str(), std::ifstream::in);
  if (!is.is_open()) {
    LOG_ERROR << "ReadFile: open file " << filename << " failed!";
    return "";
  }
  is.seekg(0, std::ifstream::end);
  int length = static_cast<int>(is.tellg());
  is.seekg(0, std::ifstream::beg);
  char *buffer = new char[length];
  is.read(buffer, length);
  is.close();
  std::string content(buffer, length);
  delete[] buffer;
  return content;
}

std::string BuildFileHtml(const std::string &dir) {
  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  std::string file;
  for (const auto &filename : files) {
    file += "<tr><td>";
    file += filename;
    file += "</td><td><a href=\"/open/";
    file += filename;
    file += "\">浏览</a><a href=\"/download/";
    file += "\">下载</a><a href=\"/delete/";
    file += "\">删除</a></td></tr>\n";
  }

  std::string tmp = "<!--filelist-->";
  std::string body = ReadFile("../static/fileserver.html");
  body.replace(body.find(tmp), tmp.length(), file);
  return body;
}

std::string BuildAnomFileHtml(const std::string &dir) {
  std::vector<std::string> files;
  FindAllFiles(dir, &files);
  std::string file;
  for (const auto &filename : files) {
    file += "<tr><td>";
    file += filename;
    file += "</td><td><a href=\"/open/";
    file += filename;
    file += "\">浏览</a>";
  }

  std::string tmp = "<!--filelist-->";
  std::string body = ReadFile("../static/anomfileserver.html");
  body.replace(body.find(tmp), tmp.length(), file);
  return body;
}

void OpenFileSystem(HttpResponse *response) {  // 不设置域名防止别人直接闯入
  response->SetContentType("text/html; charset=UTF-8");
  response->SetResponseBody(BuildFileHtml("../files"));
  response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
  response->SetStatusMessage("OK");
}

void Httpopen(const std::string &filename, HttpResponse *response) {
  // std::cout<<"open file "<<filename<<std::endl;
  if (IsFindInDir(filename, "../files/")) {
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
      response->SetResponseBody(ReadFile("../files/" + filename));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
      LOG_INFO << "Open file " << filename << " success!";
    } else {
      // std::cout<<"open file 1"<<filename<<std::endl;
      LOG_ERROR << "Open file " << filename << " failed!";
      response->SetResponseBody("Open file ");
      Relocation(response);
    }
  } else {
    // std::cout<<"open file 2"<<filename<<std::endl;
    LOG_ERROR << "Open file " << filename << " failed!";
    Relocation(response);
  }
}
void Httpdownload(const std::string &filename, HttpResponse *response) {
  if (IsFindInDir(filename, "../files/")) {
    int filefd = ::open(("../files/" + filename).c_str(), O_RDONLY);
    if (filefd < 0) {
      LOG_ERROR << "Download file " << filename << " failed!";
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
    LOG_ERROR << "Download file " << filename << " failed!";
    OpenFileSystem(response);
  }
}
void Httpdelete(const std::string &filename, HttpResponse *response) {
  if (IsFindInDir(filename, "../files/")) {
    if (remove(("../files/" + filename).c_str()) != 0) {
      LOG_ERROR << "Delete file " << filename << " failed!";
    } else {
      LOG_INFO << "Delete file " << filename << " success!";
    }
  } else {
    LOG_ERROR << "Delete file " << filename << " failed!";
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
      response->SetResponseBody(ReadFile("../static/index.html"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
    } else if (url.substr(0, 5) == "/open") {  // 不包括\0 中文未解码
      Httpopen(url.substr(6), response);       // 不带斜杠
    } else if (url.substr(0, 9) == "/download") {
      Httpdownload(url.substr(10), response);
    } else if (url.substr(0, 7) == "/delete") {
      Httpdelete(url.substr(8), response);
    } else if (url.substr(0, 5) == "/anom") {
      response->SetContentType("text/html; charset=UTF-8");
      response->SetResponseBody(BuildAnomFileHtml("../files"));
      response->SetStatusCode(HttpResponse::HttpStatusCode::K200K);
      response->SetStatusMessage("OK");
    } else if (url.substr(0, 4) == "/ret") {
      Relocation(response);
    } else {
      response->SetStatusCode(HttpResponse::HttpStatusCode::K400BADREQUEST);
      response->SetStatusMessage("BAD_RESQUEST");
      response->SetClose();
    }
  } else if (request.GetMethodString() == "POST") {  // post返回的内容需要有body
    if (request.GetURL() == "/login") {
      const std::string &body = request.GetBody();
      int user_pos = body.find("username=");
      int pass_pos = body.find("password=");

      user_pos += 9;
      pass_pos += 9;

      int and_pos = body.find('&', user_pos);
      int end_pos = body.length();
      std::string username = body.substr(user_pos, and_pos - user_pos);
      std::string password = body.substr(pass_pos, end_pos - pass_pos);
      LOG_INFO << "New message from POST client " << username << " : " << password;
      if (username == "xkx" && password == "pig") {
        LOG_INFO << username << " login success! ";
        OpenFileSystem(response);
      } else {
        LOG_INFO << username << " login failed! ";
        Relocation(response);
      }
    } else if (request.GetURL() == "/upload") {  // 上传成功才会到这里
      OpenFileSystem(response);
    } else {
      LOG_ERROR << "Post request failed!";
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
  if (argc == 2) {  // 加一个参数变echo_server
    httpserver->SetMessageCallBack(Message);
  } else {
    httpserver->SetHttpResponseCallBack(Http);
  }
  // httpserver->OnTimerEvery(3.0, Every);

  httpserver->Start();
  return 0;
}
