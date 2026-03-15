#pragma once
#include <string>
// RPC请求和响应消息内容的类型定义，以及注册反射的静态方法声明。
namespace rpc_model {
struct EchoRequest {
  std::string msg;
  static void RegisterReflection();
};

struct EchoResponse {
  std::string msg;
  static void RegisterReflection();
};
}  // namespace rpc_model
