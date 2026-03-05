#pragma once

#include "serialization_framework.h"
#include <string>
#include <cstdint>

// 获取用户请求
class GetUserRequest {
public:
    int32_t user_id = 0;
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "GetUserRequest";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &GetUserRequest::user_id);
        
        ReflectionRegistry::Instance().RegisterMessage("GetUserRequest", desc);
    }
};

// 获取用户响应
class GetUserResponse {
public:
    int32_t user_id = 0;
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = false;
    std::string status_message;  // 额外的状态信息
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "GetUserResponse";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &GetUserResponse::user_id);
        AddFieldToDescriptor(desc, "name", 2, FieldType::STRING, &GetUserResponse::name);
        AddFieldToDescriptor(desc, "email", 3, FieldType::STRING, &GetUserResponse::email);
        AddFieldToDescriptor(desc, "age", 4, FieldType::INT32, &GetUserResponse::age);
        AddFieldToDescriptor(desc, "is_active", 5, FieldType::BOOL, &GetUserResponse::is_active);
        AddFieldToDescriptor(desc, "status_message", 6, FieldType::STRING, &GetUserResponse::status_message);
        
        ReflectionRegistry::Instance().RegisterMessage("GetUserResponse", desc);
    }
};

// 创建用户请求
class CreateUserRequest {
public:
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = true;
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "CreateUserRequest";
        
        AddFieldToDescriptor(desc, "name", 1, FieldType::STRING, &CreateUserRequest::name);
        AddFieldToDescriptor(desc, "email", 2, FieldType::STRING, &CreateUserRequest::email);
        AddFieldToDescriptor(desc, "age", 3, FieldType::INT32, &CreateUserRequest::age);
        AddFieldToDescriptor(desc, "is_active", 4, FieldType::BOOL, &CreateUserRequest::is_active);
        
        ReflectionRegistry::Instance().RegisterMessage("CreateUserRequest", desc);
    }
};

// 创建用户响应
class CreateUserResponse {
public:
    int32_t user_id = 0;  // 新创建的用户ID
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = false;
    std::string created_at;  // 创建时间
    std::string status_message;
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "CreateUserResponse";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &CreateUserResponse::user_id);
        AddFieldToDescriptor(desc, "name", 2, FieldType::STRING, &CreateUserResponse::name);
        AddFieldToDescriptor(desc, "email", 3, FieldType::STRING, &CreateUserResponse::email);
        AddFieldToDescriptor(desc, "age", 4, FieldType::INT32, &CreateUserResponse::age);
        AddFieldToDescriptor(desc, "is_active", 5, FieldType::BOOL, &CreateUserResponse::is_active);
        AddFieldToDescriptor(desc, "created_at", 6, FieldType::STRING, &CreateUserResponse::created_at);
        AddFieldToDescriptor(desc, "status_message", 7, FieldType::STRING, &CreateUserResponse::status_message);
        
        ReflectionRegistry::Instance().RegisterMessage("CreateUserResponse", desc);
    }
};

// 更新用户请求
class UpdateUserRequest {
public:
    int32_t user_id = 0;
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = false;
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "UpdateUserRequest";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &UpdateUserRequest::user_id);
        AddFieldToDescriptor(desc, "name", 2, FieldType::STRING, &UpdateUserRequest::name);
        AddFieldToDescriptor(desc, "email", 3, FieldType::STRING, &UpdateUserRequest::email);
        AddFieldToDescriptor(desc, "age", 4, FieldType::INT32, &UpdateUserRequest::age);
        AddFieldToDescriptor(desc, "is_active", 5, FieldType::BOOL, &UpdateUserRequest::is_active);
        
        ReflectionRegistry::Instance().RegisterMessage("UpdateUserRequest", desc);
    }
};

// 更新用户响应
class UpdateUserResponse {
public:
    int32_t user_id = 0;
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = false;
    std::string updated_at;  // 更新时间
    std::string status_message;
    
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "UpdateUserResponse";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &UpdateUserResponse::user_id);
        AddFieldToDescriptor(desc, "name", 2, FieldType::STRING, &UpdateUserResponse::name);
        AddFieldToDescriptor(desc, "email", 3, FieldType::STRING, &UpdateUserResponse::email);
        AddFieldToDescriptor(desc, "age", 4, FieldType::INT32, &UpdateUserResponse::age);
        AddFieldToDescriptor(desc, "is_active", 5, FieldType::BOOL, &UpdateUserResponse::is_active);
        AddFieldToDescriptor(desc, "updated_at", 6, FieldType::STRING, &UpdateUserResponse::updated_at);
        AddFieldToDescriptor(desc, "status_message", 7, FieldType::STRING, &UpdateUserResponse::status_message);
        
        ReflectionRegistry::Instance().RegisterMessage("UpdateUserResponse", desc);
    }
}; 