#pragma once
#include <string>

namespace my_socket_rpc { // 仅前置声明，避免引入过多依赖
class RpcRequest;
class RpcResponse;
class EchoRequest;
class EchoResponse;
}  // namespace my_socket_rpc

namespace rpcproto {
bool DecodeRpcRequest(const std::string &data, my_socket_rpc::RpcRequest *out);
bool EncodeRpcRequest(const my_socket_rpc::RpcRequest &req, std::string *out);
bool EncodeRpcResponse(const my_socket_rpc::RpcResponse &resp, std::string *out);
bool DecodeEchoRequest(const std::string &data, my_socket_rpc::EchoRequest *out);
bool EncodeEchoRequest(const my_socket_rpc::EchoRequest &req, std::string *out);
bool EncodeEchoResponse(const my_socket_rpc::EchoResponse &resp, std::string *out);
bool DecodeEchoResponse(const std::string &data, my_socket_rpc::EchoResponse *out);
}  // namespace rpcproto
