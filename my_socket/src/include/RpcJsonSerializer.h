#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "RpcSerialization.h"

class RpcJsonSerializer : public ISerializer {
 public:
  bool Serialize(const void *obj, const RpcMessageDescriptor &desc, std::string *output) override {
    if (!obj || !output) {
      return false;
    }
    try {
      nlohmann::json root = nlohmann::json::object();
      for (const auto &field : desc.fields) {
        std::any value = field.getter(obj);
        switch (field.type) {
          case RpcFieldType::INT32:
            root[field.name] = std::any_cast<int32_t>(value);
            break;
          case RpcFieldType::INT64:
            root[field.name] = std::any_cast<int64_t>(value);
            break;
          case RpcFieldType::STRING:
            root[field.name] = std::any_cast<std::string>(value);
            break;
          case RpcFieldType::BOOL:
            root[field.name] = std::any_cast<bool>(value);
            break;
          case RpcFieldType::BYTES:
            root[field.name] = std::any_cast<std::string>(value);
            break;
          default:
            return false;
        }
      }
      *output = root.dump();
      return true;
    } catch (...) {
      return false;
    }
  }

  bool Deserialize(const std::string &input, void *obj, const RpcMessageDescriptor &desc) override {
    if (!obj) {
      return false;
    }
    try {
      nlohmann::json root = nlohmann::json::parse(input);
      for (const auto &field : desc.fields) {
        if (!root.contains(field.name)) {
          continue;
        }
        std::any value;
        switch (field.type) {
          case RpcFieldType::INT32:
            value = root[field.name].get<int32_t>();
            break;
          case RpcFieldType::INT64:
            value = root[field.name].get<int64_t>();
            break;
          case RpcFieldType::STRING:
            value = root[field.name].get<std::string>();
            break;
          case RpcFieldType::BOOL:
            value = root[field.name].get<bool>();
            break;
          case RpcFieldType::BYTES:
            value = root[field.name].get<std::string>();
            break;
          default:
            continue;
        }
        field.setter(obj, value);
      }
      return true;
    } catch (...) {
      return false;
    }
  }

  std::string GetName() const override { return "json"; }
};
