#include "RpcEchoTypes.h"
#include "RpcSerialization.h"

namespace rpc_model {
void EchoRequest::RegisterReflection() {  // 将所有元素添加到描述符中，并f封装成信息注册到反射注册表中
  RpcMessageDescriptor desc;
  desc.message_name = "EchoRequest";
  AddFieldToDescriptor(&desc, "msg", RpcFieldType::STRING, &EchoRequest::msg);
  ReflectionRegistry::Instance().RegisterMessage(desc.message_name, std::move(desc));
}

void EchoResponse::RegisterReflection() {
  RpcMessageDescriptor desc;
  desc.message_name = "EchoResponse";
  AddFieldToDescriptor(&desc, "msg", RpcFieldType::STRING, &EchoResponse::msg);
  ReflectionRegistry::Instance().RegisterMessage(desc.message_name, std::move(desc));
}
}  // namespace rpc_model
