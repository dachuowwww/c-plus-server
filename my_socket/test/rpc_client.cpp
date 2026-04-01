#include <unistd.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <coroutine>
#include "RpcProto.h"
#include "Socket.h"
#include "rpc.pb.h"

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

struct RpcCallResult {
  int req_id = 0;
  bool transport_ok = false;
  my_socket_rpc::RpcResponse rpc_resp;
  std::string echo_msg;
  std::string error_message;
};

bool BuildEchoPayload(const std::string &serializer, const std::string &msg, std::string *payload) {
  if (!payload) {
    return false;
  }
  if (serializer == "protobuf") {
    my_socket_rpc::EchoRequest echo_req;
    echo_req.set_msg(msg);
    return rpcproto::EncodeEchoRequest(echo_req, payload);
  }
  if (serializer == "json") {
    nlohmann::json j;
    j["msg"] = msg;
    *payload = j.dump();
    return true;
  }
  return false;
}

RpcCallResult SendOneRpc(const std::string &ip, int port, int req_id, const std::string &msg,
                         const std::string &serializer) {  // 协程函数实际等待操作
  RpcCallResult result;
  result.req_id = req_id;

  std::string echo_result;
  if (!BuildEchoPayload(serializer, msg, &echo_result)) {
    result.error_message = "build echo payload failed";
    return result;
  }

  my_socket_rpc::RpcRequest rpc_req;
  rpc_req.set_id(req_id);
  rpc_req.set_method("Echo");
  rpc_req.set_params(echo_result);
  rpc_req.set_serializer_type(serializer);

  std::string body;
  if (!rpcproto::EncodeRpcRequest(rpc_req, &body)) {
    result.error_message = "encode rpc request failed";
    return result;
  }

  std::string request;
  request += "POST /rpc HTTP/1.1\r\n";
  request += "Host: " + ip + ":" + std::to_string(port) + "\r\n";
  request += "Content-Type: application/x-protobuf\r\n";
  // request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  request += body;

  Socket sock;
  sock.Connect(ip.c_str(), static_cast<uint16_t>(port));
  if (!SendAll(sock.GetFd(), request)) {
    result.error_message = std::string("send request failed: ") + std::strerror(errno);
    return result;
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
    result.error_message = std::string("read response failed: ") + std::strerror(errno);
    return result;
  }

  size_t header_end = response.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    result.error_message = "invalid http response";
    return result;
  }

  std::string body_resp = response.substr(header_end + 4);
  if (!result.rpc_resp.ParseFromString(body_resp)) {
    result.error_message = "decode rpc response failed";
    return result;
  }
  result.transport_ok = true;

  if (!result.rpc_resp.ok()) {
    return result;
  }
  if (result.rpc_resp.serializer_type() == "protobuf") {
    my_socket_rpc::EchoResponse echo_resp;
    if (!rpcproto::DecodeEchoResponse(result.rpc_resp.params(), &echo_resp)) {
      result.error_message = "decode protobuf echo response failed";
      return result;
    }
    result.echo_msg = echo_resp.msg();
    return result;
  }
  if (result.rpc_resp.serializer_type() == "json") {
    try {
      nlohmann::json j = nlohmann::json::parse(result.rpc_resp.params());
      result.echo_msg = j.value("msg", "");
    } catch (...) {
      result.error_message = "decode json response failed";
    }
  }
  return result;
}

template <typename T>
struct FutureAwaiter {           // 协程等待体
  std::shared_future<T> future;  // 用列表初始化传入worker

  [[nodiscard]] bool await_ready() const {
    return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }

  void await_suspend(std::coroutine_handle<> handle) const {
    std::thread([f = future, handle]() mutable {
      f.wait();
      handle.resume();
    }).detach();
  }

  T await_resume() const { return future.get(); }
};

template <typename T>
class CoroutineTask {  // 协程体
 public:
  struct promise_type {
    std::promise<T> promise;

    CoroutineTask get_return_object() { return CoroutineTask(promise.get_future()); }
    std::suspend_never initial_suspend() noexcept { return {}; }         // 直接开始协程函数体
    std::suspend_never final_suspend() noexcept { return {}; }           // 自动结束
    void return_value(T value) { promise.set_value(std::move(value)); }  // 协程函数体通过co_return返回结果
    void unhandled_exception() {
      promise.set_exception(std::current_exception());
    }  // 把异常塞进 promise，对应的 future 在 .get() 时会重新抛出。
  };

  explicit CoroutineTask(std::future<T> &&future) : future_(std::move(future)) {}
  CoroutineTask(CoroutineTask &&) noexcept = default;
  CoroutineTask &operator=(CoroutineTask &&) noexcept = default;

  std::future<T> TakeFuture() { return std::move(future_); }

 private:
  std::future<T> future_;
};

CoroutineTask<RpcCallResult> SendOneRpcByCoroutine(const std::string &ip, int port, int req_id,
                                                   const std::string &msg,  // 协程函数
                                                   const std::string &serializer) {
  std::shared_future<RpcCallResult> worker =
      std::async(std::launch::async,  // 单开线程 返回future，协程等待这个future完成
                 [ip, port, req_id, msg, serializer]() { return SendOneRpc(ip, port, req_id, msg, serializer); })
          .share();  // 转成 shared_future 以支持在 FutureAwaiter 中复制和移动
  RpcCallResult result = co_await FutureAwaiter<RpcCallResult>{worker};
  co_return result;
}

void PrintRpcResult(const RpcCallResult &result) {
  if (!result.transport_ok) {
    std::cout << "rpc id=" << result.req_id << " transport failed, error=" << result.error_message << std::endl;
    return;
  }
  std::cout << "rpc id=" << result.rpc_resp.id() << " ok=" << result.rpc_resp.ok() << " code=" << result.rpc_resp.code()
            << " message=" << result.rpc_resp.message() << " serializer=" << result.rpc_resp.serializer_type()
            << std::endl;
  if (!result.echo_msg.empty()) {
    std::cout << "echo msg=" << result.echo_msg << std::endl;
  }
  if (!result.error_message.empty()) {
    std::cout << "note: " << result.error_message << std::endl;
  }
}
}  // namespace

int main(int argc, char *argv[]) {
  std::string ip = "127.0.0.1";
  int port = 80;
  std::array<std::string, 3> msgs = {"hi_1", "hi_2", "hi_3"};
  std::string serializer = "protobuf";

  if (argc >= 2) {
    ip = argv[1];
  }
  if (argc >= 3) {
    port = std::atoi(argv[2]);
  }
  if (argc >= 5) {  // 兼容旧参数：ip port msg serializer
    msgs[0] = argv[3];
    msgs[1] = std::string(argv[3]) + "_2";
    msgs[2] = std::string(argv[3]) + "_3";
    serializer = argv[4];
  }
  if (argc >= 7) {  // 新参数：ip port msg1 msg2 msg3 serializer
    msgs[0] = argv[3];
    msgs[1] = argv[4];
    msgs[2] = argv[5];
    serializer = argv[6];
  }

  if (serializer != "protobuf" && serializer != "json") {
    std::cerr << "unsupported serializer: " << serializer << std::endl;
    return 1;
  }

  std::cout << "send 3 rpc requests asynchronously..." << std::endl;
  auto task1 = SendOneRpcByCoroutine(ip, port, 1, msgs[0], serializer);
  auto task2 = SendOneRpcByCoroutine(ip, port, 2, msgs[1], serializer);
  auto task3 = SendOneRpcByCoroutine(ip, port, 3, msgs[2], serializer);

  std::array<std::future<RpcCallResult>, 3> futures = {
      task1.TakeFuture(),
      task2.TakeFuture(),
      task3.TakeFuture(),
  };
  std::array<bool, 3> finished = {false, false, false};
  int finished_count = 0;
  int failed_count = 0;
  while (finished_count < 3) {
    bool has_progress = false;
    for (size_t i = 0; i < futures.size(); ++i) {
      if (finished[i]) {
        continue;
      }
      if (futures[i].wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        RpcCallResult result = futures[i].get();
        PrintRpcResult(result);
        if (!result.transport_ok) {
          ++failed_count;
        }
        finished[i] = true;
        ++finished_count;
        has_progress = true;
      }
    }
    if (!has_progress) {  // 没结果则等待
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  // std::string msg = "hi";
  // if (argc >= 4) {
  //   msg = argv[3];
  // }
  // if (argc >= 5) {
  //   serializer = argv[4];
  // }
  // std::string echo_result;
  // if (serializer == "protobuf") {
  //   my_socket_rpc::EchoRequest echo_req;  // 对象
  //   echo_req.set_msg(msg);
  //   if (!rpcproto::EncodeEchoRequest(echo_req, &echo_result)) {
  //     std::cerr << "encode echo request failed" << std::endl;
  //     return 1;
  //   }
  // } else if (serializer == "json") {
  //   nlohmann::json j;
  //   j["msg"] = msg;
  //   echo_result = j.dump();
  // } else {
  //   std::cerr << "unsupported serializer: " << serializer << std::endl;
  //   return 1;
  // }
  // my_socket_rpc::RpcRequest rpc_req;
  // rpc_req.set_id(1);
  // rpc_req.set_method("Echo");
  // rpc_req.set_params(echo_result);
  // rpc_req.set_serializer_type(serializer);
  // std::string body;
  // if (!rpcproto::EncodeRpcRequest(rpc_req, &body)) {
  //   std::cerr << "encode rpc request failed" << std::endl;
  //   return 1;
  // }
  // std::string request;
  // request += "POST /rpc HTTP/1.1\r\n";
  // request += "Host: " + ip + ":" + std::to_string(port) + "\r\n";
  // request += "Content-Type: application/x-protobuf\r\n";
  // request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
  // request += "Connection: close\r\n";
  // request += "\r\n";
  // request += body;
  // Socket sock;
  // sock.Connect(ip.c_str(), static_cast<uint16_t>(port));
  // if (!SendAll(sock.GetFd(), request)) {
  //   std::cerr << "send request failed: " << std::strerror(errno) << std::endl;
  //   return 1;
  // }
  // std::string response;
  // char buf[1024];
  // while (true) {
  //   ssize_t n = ::read(sock.GetFd(), buf, sizeof(buf));
  //   if (n > 0) {
  //     response.append(buf, static_cast<size_t>(n));
  //     continue;
  //   }
  //   if (n == 0) {
  //     break;
  //   }
  //   if (errno == EINTR) {
  //     continue;
  //   }
  //   std::cerr << "read response failed: " << std::strerror(errno) << std::endl;
  //   break;
  // }
  // std::cout << response << std::endl;
  // size_t header_end = response.find("\r\n\r\n");
  // if (header_end == std::string::npos) {
  //   return 0;
  // }
  // std::string body_resp = response.substr(header_end + 4);
  // my_socket_rpc::RpcResponse rpc_resp;
  // if (!rpc_resp.ParseFromString(body_resp)) {
  //   std::cerr << "decode rpc response failed" << std::endl;
  //   return 0;
  // }
  // std::cout << "rpc id=" << rpc_resp.id() << " ok=" << rpc_resp.ok() << " code=" << rpc_resp.code()
  //           << " message=" << rpc_resp.message() << " serializer=" << rpc_resp.serializer_type() << std::endl;
  // if (rpc_resp.ok()) {
  //   if (rpc_resp.serializer_type() == "protobuf") {
  //     my_socket_rpc::EchoResponse echo_resp;
  //     if (rpcproto::DecodeEchoResponse(rpc_resp.params(), &echo_resp)) {
  //       std::cout << "echo msg=" << echo_resp.msg() << std::endl;
  //     }
  //   } else if (rpc_resp.serializer_type() == "json") {
  //     try {
  //       nlohmann::json j = nlohmann::json::parse(rpc_resp.params());
  //       std::cout << "echo msg=" << j.value("msg", "") << std::endl;
  //     } catch (...) {
  //       std::cerr << "decode json response failed" << std::endl;
  //     }
  //   }
  // }

  if (failed_count > 0) {
    return 1;
  }
  return 0;
}
