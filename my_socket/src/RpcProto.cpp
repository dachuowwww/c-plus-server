#include "RpcProto.h"
#include "rpc.pb.h"

namespace rpcproto {
bool DecodeRpcRequest(const std::string &data, my_socket_rpc::RpcRequest *out) {
  if (!out) {
    return false;
  }
  return out->ParseFromString(data);
}

bool EncodeRpcRequest(const my_socket_rpc::RpcRequest &req, std::string *out) {
  if (!out) {
    return false;
  }
  return req.SerializeToString(out);
}

bool EncodeRpcResponse(const my_socket_rpc::RpcResponse &resp, std::string *out) {
  if (!out) {
    return false;
  }
  return resp.SerializeToString(out);
}

bool DecodeEchoRequest(const std::string &data, my_socket_rpc::EchoRequest *out) {
  if (!out) {
    return false;
  }
  return out->ParseFromString(data);
}

bool EncodeEchoRequest(const my_socket_rpc::EchoRequest &req, std::string *out) {
  if (!out) {
    return false;
  }
  return req.SerializeToString(out);
}

bool EncodeEchoResponse(const my_socket_rpc::EchoResponse &resp, std::string *out) {
  if (!out) {
    return false;
  }
  return resp.SerializeToString(out);
}

bool DecodeEchoResponse(const std::string &data, my_socket_rpc::EchoResponse *out) {
  if (!out) {
    return false;
  }
  return out->ParseFromString(data);
}
}  // namespace rpcproto
