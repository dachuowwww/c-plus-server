#include "Socket.h"
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>  // 引入json库
#include <string>


namespace {
bool SendAll(int fd, const std::string &data) {
  size_t sent = 0;
  while (sent < data.size()) {
    ssize_t n = ::write(fd, data.data() + sent, data.size() - sent);
    if (n > 0) {
      sent += static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR) {
      continue;
    }
    return false;
  }
  return true;
}
}  // namespace

int main(int argc, char *argv[]) {
  std::string ip = "127.0.0.1";
  int port = 80;
  std::string msg = "hi";

  if (argc >= 2) {
    ip = argv[1];
  }
  if (argc >= 3) {
    port = std::atoi(argv[2]);
  }
  if (argc >= 4) {
    msg = argv[3];
  }

  nlohmann::json request_json;
  request_json["id"] = 1;
  request_json["method"] = "Echo";
  request_json["params"] = {{"msg", msg}};
  std::string body = request_json.dump();
  std::string request;
  request += "POST /rpc HTTP/1.1\r\n";
  request += "Host: " + ip + ":" + std::to_string(port) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  request += body;

  Socket sock;
  sock.Connect(ip.c_str(), static_cast<uint16_t>(port));
  if (!SendAll(sock.GetFd(), request)) {
    std::cerr << "send request failed: " << std::strerror(errno) << std::endl;
    return 1;
  }

  std::string response;
  char buf[1024];
  while (true) {
    ssize_t n = ::read(sock.GetFd(), buf, sizeof(buf));
    if (n > 0) {
      response.append(buf, static_cast<size_t>(n));
      continue;
    }
    if (n == 0) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    std::cerr << "read response failed: " << std::strerror(errno) << std::endl;
    break;
  }

  std::cout << response << std::endl;
  return 0;
}
