#pragma once

#include "serialization_framework.h"
#include <string>
#include <cstdint>

// 用户信息类
class UserInfo {
public:
    int32_t user_id = 0;
    std::string name;
    std::string email;
    int32_t age = 0;
    bool is_active = false;
    
    // 注册反射信息（用于通用序列化器）
    static void RegisterReflection() {
        MessageDescriptor desc;
        desc.message_name = "UserInfo";
        
        AddFieldToDescriptor(desc, "user_id", 1, FieldType::INT32, &UserInfo::user_id);
        AddFieldToDescriptor(desc, "name", 2, FieldType::STRING, &UserInfo::name);
        AddFieldToDescriptor(desc, "email", 3, FieldType::STRING, &UserInfo::email);
        AddFieldToDescriptor(desc, "age", 4, FieldType::INT32, &UserInfo::age);
        AddFieldToDescriptor(desc, "is_active", 5, FieldType::BOOL, &UserInfo::is_active);
        
        ReflectionRegistry::Instance().RegisterMessage("UserInfo", desc);
    }
}; 