#pragma once

#include "serialization_framework.h"
#include "user_info.h"
// #include "user.pb.h"
#include <json/json.h>

// UserInfo的JSON序列化器
class UserInfoJsonSerializer : public ITypeSerializer<UserInfo> {
public:
    bool Serialize(const UserInfo& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UserInfo& obj) override;
    std::string GetName() const override { return "json"; }
};

// UserInfo的Protobuf序列化器
class UserInfoProtobufSerializer : public ITypeSerializer<UserInfo> {
public:
    bool Serialize(const UserInfo& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UserInfo& obj) override;
    std::string GetName() const override { return "protobuf"; }
}; 