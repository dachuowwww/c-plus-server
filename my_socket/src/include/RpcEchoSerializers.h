#pragma once
#include <string>
#include "RpcEchoTypes.h"
#include "RpcSerialization.h"


class EchoRequestJsonSerializer : public ITypeSerializer<rpc_model::EchoRequest> {
 public:
  bool Serialize(const rpc_model::EchoRequest &obj, std::string *output) override;
  bool Deserialize(const std::string &input, rpc_model::EchoRequest *obj) override;
  std::string GetName() const override { return "json"; }
};

class EchoResponseJsonSerializer : public ITypeSerializer<rpc_model::EchoResponse> {
 public:
  bool Serialize(const rpc_model::EchoResponse &obj, std::string *output) override;
  bool Deserialize(const std::string &input, rpc_model::EchoResponse *obj) override;
  std::string GetName() const override { return "json"; }
};

class EchoRequestProtobufSerializer : public ITypeSerializer<rpc_model::EchoRequest> {
 public:
  bool Serialize(const rpc_model::EchoRequest &obj, std::string *output) override;
  bool Deserialize(const std::string &input, rpc_model::EchoRequest *obj) override;
  std::string GetName() const override { return "protobuf"; }
};

class EchoResponseProtobufSerializer : public ITypeSerializer<rpc_model::EchoResponse> {
 public:
  bool Serialize(const rpc_model::EchoResponse &obj, std::string *output) override;
  bool Deserialize(const std::string &input, rpc_model::EchoResponse *obj) override;
  std::string GetName() const override { return "protobuf"; }
};
