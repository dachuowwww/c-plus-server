#pragma once

#include "serialization_framework.h"
#include "user_types.h"
#include <json/json.h>

// GetUserRequest 序列化器
class GetUserRequestJsonSerializer : public ITypeSerializer<GetUserRequest> {
public:
    bool Serialize(const GetUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, GetUserRequest& obj) override;
    std::string GetName() const override { return "json"; }
};

class GetUserRequestProtobufSerializer : public ITypeSerializer<GetUserRequest> {
public:
    bool Serialize(const GetUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, GetUserRequest& obj) override;
    std::string GetName() const override { return "protobuf"; }
};

// GetUserResponse 序列化器
class GetUserResponseJsonSerializer : public ITypeSerializer<GetUserResponse> {
public:
    bool Serialize(const GetUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, GetUserResponse& obj) override;
    std::string GetName() const override { return "json"; }
};

class GetUserResponseProtobufSerializer : public ITypeSerializer<GetUserResponse> {
public:
    bool Serialize(const GetUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, GetUserResponse& obj) override;
    std::string GetName() const override { return "protobuf"; }
};

// CreateUserRequest 序列化器
class CreateUserRequestJsonSerializer : public ITypeSerializer<CreateUserRequest> {
public:
    bool Serialize(const CreateUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, CreateUserRequest& obj) override;
    std::string GetName() const override { return "json"; }
};

class CreateUserRequestProtobufSerializer : public ITypeSerializer<CreateUserRequest> {
public:
    bool Serialize(const CreateUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, CreateUserRequest& obj) override;
    std::string GetName() const override { return "protobuf"; }
};

// CreateUserResponse 序列化器
class CreateUserResponseJsonSerializer : public ITypeSerializer<CreateUserResponse> {
public:
    bool Serialize(const CreateUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, CreateUserResponse& obj) override;
    std::string GetName() const override { return "json"; }
};

class CreateUserResponseProtobufSerializer : public ITypeSerializer<CreateUserResponse> {
public:
    bool Serialize(const CreateUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, CreateUserResponse& obj) override;
    std::string GetName() const override { return "protobuf"; }
};

// UpdateUserRequest 序列化器
class UpdateUserRequestJsonSerializer : public ITypeSerializer<UpdateUserRequest> {
public:
    bool Serialize(const UpdateUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UpdateUserRequest& obj) override;
    std::string GetName() const override { return "json"; }
};

class UpdateUserRequestProtobufSerializer : public ITypeSerializer<UpdateUserRequest> {
public:
    bool Serialize(const UpdateUserRequest& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UpdateUserRequest& obj) override;
    std::string GetName() const override { return "protobuf"; }
};

// UpdateUserResponse 序列化器
class UpdateUserResponseJsonSerializer : public ITypeSerializer<UpdateUserResponse> {
public:
    bool Serialize(const UpdateUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UpdateUserResponse& obj) override;
    std::string GetName() const override { return "json"; }
};

class UpdateUserResponseProtobufSerializer : public ITypeSerializer<UpdateUserResponse> {
public:
    bool Serialize(const UpdateUserResponse& obj, std::string& output) override;
    bool Deserialize(const std::string& input, UpdateUserResponse& obj) override;
    std::string GetName() const override { return "protobuf"; }
}; 