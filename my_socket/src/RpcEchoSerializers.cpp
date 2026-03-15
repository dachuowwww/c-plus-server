#include "RpcEchoSerializers.h"
#include <nlohmann/json.hpp>
#include "rpc.pb.h"

bool EchoRequestJsonSerializer::Serialize(const rpc_model::EchoRequest &obj, std::string *output) {
  if (!output) {
    return false;
  }
  nlohmann::json root;
  root["msg"] = obj.msg;
  *output = root.dump();
  return true;
}

bool EchoRequestJsonSerializer::Deserialize(const std::string &input, rpc_model::EchoRequest *obj) {
  if (!obj) {
    return false;
  }
  try {
    nlohmann::json root = nlohmann::json::parse(input);
    obj->msg = root.value("msg", "");
    return true;
  } catch (...) {
    return false;
  }
}

bool EchoResponseJsonSerializer::Serialize(const rpc_model::EchoResponse &obj, std::string *output) {
  if (!output) {
    return false;
  }
  nlohmann::json root;
  root["msg"] = obj.msg;
  *output = root.dump();
  return true;
}

bool EchoResponseJsonSerializer::Deserialize(const std::string &input, rpc_model::EchoResponse *obj) {
  if (!obj) {
    return false;
  }
  try {
    nlohmann::json root = nlohmann::json::parse(input);
    obj->msg = root.value("msg", "");
    return true;
  } catch (...) {
    return false;
  }
}

bool EchoRequestProtobufSerializer::Serialize(const rpc_model::EchoRequest &obj, std::string *output) {
  if (!output) {
    return false;
  }
  my_socket_rpc::EchoRequest req;
  req.set_msg(obj.msg);
  return req.SerializeToString(output);
}

bool EchoRequestProtobufSerializer::Deserialize(const std::string &input, rpc_model::EchoRequest *obj) {
  if (!obj) {
    return false;
  }
  my_socket_rpc::EchoRequest req;
  if (!req.ParseFromString(input)) {
    return false;
  }
  obj->msg = req.msg();
  return true;
}

bool EchoResponseProtobufSerializer::Serialize(const rpc_model::EchoResponse &obj, std::string *output) {
  if (!output) {
    return false;
  }
  my_socket_rpc::EchoResponse resp;
  resp.set_msg(obj.msg);
  return resp.SerializeToString(output);
}

bool EchoResponseProtobufSerializer::Deserialize(const std::string &input, rpc_model::EchoResponse *obj) {
  if (!obj) {
    return false;
  }
  my_socket_rpc::EchoResponse resp;
  if (!resp.ParseFromString(input)) {
    return false;
  }
  obj->msg = resp.msg();
  return true;
}
