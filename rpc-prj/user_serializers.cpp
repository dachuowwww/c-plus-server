#include "user_serializers.h"
#include <sstream>
#include <iostream>

#include "user.pb.h"

// UserInfo JSON序列化器实现
bool UserInfoJsonSerializer::Serialize(const UserInfo& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";  // 紧凑格式
        output = Json::writeString(builder, root);
        
        std::cout << "UserInfo JSON Serializer: Serialized to " << output << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UserInfo JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UserInfoJsonSerializer::Deserialize(const std::string& input, UserInfo& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream stream(input);
        if (!Json::parseFromStream(builder, stream, &root, &errors)) {
            std::cerr << "JSON parse error: " << errors << std::endl;
            return false;
        }
        
        obj.user_id = root.get("user_id", 0).asInt();
        obj.name = root.get("name", "").asString();
        obj.email = root.get("email", "").asString();
        obj.age = root.get("age", 0).asInt();
        obj.is_active = root.get("is_active", false).asBool();
        
        std::cout << "UserInfo JSON Serializer: Deserialized from " << input << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UserInfo JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// UserInfo Protobuf序列化器实现
bool UserInfoProtobufSerializer::Serialize(const UserInfo& obj, std::string& output) {
    try {
        rpc_example::UserInfo pb_msg;
        
        // 将UserInfo对象的字段复制到protobuf消息
        pb_msg.set_user_id(obj.user_id);
        pb_msg.set_name(obj.name);
        pb_msg.set_email(obj.email);
        pb_msg.set_age(obj.age);
        pb_msg.set_is_active(obj.is_active);
        
        bool result = pb_msg.SerializeToString(&output);
        if (result) {
            std::cout << "UserInfo Protobuf Serializer: Serialized " << pb_msg.ShortDebugString() << std::endl;
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "UserInfo Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UserInfoProtobufSerializer::Deserialize(const std::string& input, UserInfo& obj) {
    try {
        rpc_example::UserInfo pb_msg;
        
        if (!pb_msg.ParseFromString(input)) {
            std::cerr << "Failed to parse protobuf message" << std::endl;
            return false;
        }
        
        // 将protobuf消息的字段复制到UserInfo对象
        obj.user_id = pb_msg.user_id();
        obj.name = pb_msg.name();
        obj.email = pb_msg.email();
        obj.age = pb_msg.age();
        obj.is_active = pb_msg.is_active();
        
        std::cout << "UserInfo Protobuf Serializer: Deserialized " << pb_msg.ShortDebugString() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UserInfo Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
} 