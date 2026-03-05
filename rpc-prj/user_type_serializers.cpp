#include "user_type_serializers.h"
#include "user.pb.h"
#include <json/json.h>
#include <sstream>

// GetUserRequest JSON序列化器实现
bool GetUserRequestJsonSerializer::Serialize(const GetUserRequest& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserRequest JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool GetUserRequestJsonSerializer::Deserialize(const std::string& input, GetUserRequest& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "GetUserRequest JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("user_id")) {
            obj.user_id = root["user_id"].asInt();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserRequest JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// GetUserRequest Protobuf序列化器实现
bool GetUserRequestProtobufSerializer::Serialize(const GetUserRequest& obj, std::string& output) {
    try {
        rpc_example::GetUserRequest pb_obj;
        pb_obj.set_user_id(obj.user_id);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "GetUserRequest Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool GetUserRequestProtobufSerializer::Deserialize(const std::string& input, GetUserRequest& obj) {
    try {
        rpc_example::GetUserRequest pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.user_id = pb_obj.user_id();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserRequest Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// GetUserResponse JSON序列化器实现
bool GetUserResponseJsonSerializer::Serialize(const GetUserResponse& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        root["status_message"] = obj.status_message;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserResponse JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool GetUserResponseJsonSerializer::Deserialize(const std::string& input, GetUserResponse& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "GetUserResponse JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("user_id")) obj.user_id = root["user_id"].asInt();
        if (root.isMember("name")) obj.name = root["name"].asString();
        if (root.isMember("email")) obj.email = root["email"].asString();
        if (root.isMember("age")) obj.age = root["age"].asInt();
        if (root.isMember("is_active")) obj.is_active = root["is_active"].asBool();
        if (root.isMember("status_message")) obj.status_message = root["status_message"].asString();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserResponse JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// GetUserResponse Protobuf序列化器实现
bool GetUserResponseProtobufSerializer::Serialize(const GetUserResponse& obj, std::string& output) {
    try {
        rpc_example::GetUserResponse pb_obj;
        pb_obj.set_user_id(obj.user_id);
        pb_obj.set_name(obj.name);
        pb_obj.set_email(obj.email);
        pb_obj.set_age(obj.age);
        pb_obj.set_is_active(obj.is_active);
        pb_obj.set_status_message(obj.status_message);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "GetUserResponse Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool GetUserResponseProtobufSerializer::Deserialize(const std::string& input, GetUserResponse& obj) {
    try {
        rpc_example::GetUserResponse pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.user_id = pb_obj.user_id();
        obj.name = pb_obj.name();
        obj.email = pb_obj.email();
        obj.age = pb_obj.age();
        obj.is_active = pb_obj.is_active();
        obj.status_message = pb_obj.status_message();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "GetUserResponse Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// CreateUserRequest JSON序列化器实现
bool CreateUserRequestJsonSerializer::Serialize(const CreateUserRequest& obj, std::string& output) {
    try {
        Json::Value root;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserRequest JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool CreateUserRequestJsonSerializer::Deserialize(const std::string& input, CreateUserRequest& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "CreateUserRequest JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("name")) obj.name = root["name"].asString();
        if (root.isMember("email")) obj.email = root["email"].asString();
        if (root.isMember("age")) obj.age = root["age"].asInt();
        if (root.isMember("is_active")) obj.is_active = root["is_active"].asBool();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserRequest JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// CreateUserRequest Protobuf序列化器实现
bool CreateUserRequestProtobufSerializer::Serialize(const CreateUserRequest& obj, std::string& output) {
    try {
        rpc_example::CreateUserRequest pb_obj;
        pb_obj.set_name(obj.name);
        pb_obj.set_email(obj.email);
        pb_obj.set_age(obj.age);
        pb_obj.set_is_active(obj.is_active);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "CreateUserRequest Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool CreateUserRequestProtobufSerializer::Deserialize(const std::string& input, CreateUserRequest& obj) {
    try {
        rpc_example::CreateUserRequest pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.name = pb_obj.name();
        obj.email = pb_obj.email();
        obj.age = pb_obj.age();
        obj.is_active = pb_obj.is_active();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserRequest Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// CreateUserResponse JSON序列化器实现
bool CreateUserResponseJsonSerializer::Serialize(const CreateUserResponse& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        root["created_at"] = obj.created_at;
        root["status_message"] = obj.status_message;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserResponse JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool CreateUserResponseJsonSerializer::Deserialize(const std::string& input, CreateUserResponse& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "CreateUserResponse JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("user_id")) obj.user_id = root["user_id"].asInt();
        if (root.isMember("name")) obj.name = root["name"].asString();
        if (root.isMember("email")) obj.email = root["email"].asString();
        if (root.isMember("age")) obj.age = root["age"].asInt();
        if (root.isMember("is_active")) obj.is_active = root["is_active"].asBool();
        if (root.isMember("created_at")) obj.created_at = root["created_at"].asString();
        if (root.isMember("status_message")) obj.status_message = root["status_message"].asString();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserResponse JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// CreateUserResponse Protobuf序列化器实现
bool CreateUserResponseProtobufSerializer::Serialize(const CreateUserResponse& obj, std::string& output) {
    try {
        rpc_example::CreateUserResponse pb_obj;
        pb_obj.set_user_id(obj.user_id);
        pb_obj.set_name(obj.name);
        pb_obj.set_email(obj.email);
        pb_obj.set_age(obj.age);
        pb_obj.set_is_active(obj.is_active);
        pb_obj.set_created_at(obj.created_at);
        pb_obj.set_status_message(obj.status_message);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "CreateUserResponse Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool CreateUserResponseProtobufSerializer::Deserialize(const std::string& input, CreateUserResponse& obj) {
    try {
        rpc_example::CreateUserResponse pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.user_id = pb_obj.user_id();
        obj.name = pb_obj.name();
        obj.email = pb_obj.email();
        obj.age = pb_obj.age();
        obj.is_active = pb_obj.is_active();
        obj.created_at = pb_obj.created_at();
        obj.status_message = pb_obj.status_message();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CreateUserResponse Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// UpdateUserRequest JSON序列化器实现
bool UpdateUserRequestJsonSerializer::Serialize(const UpdateUserRequest& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserRequest JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UpdateUserRequestJsonSerializer::Deserialize(const std::string& input, UpdateUserRequest& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "UpdateUserRequest JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("user_id")) obj.user_id = root["user_id"].asInt();
        if (root.isMember("name")) obj.name = root["name"].asString();
        if (root.isMember("email")) obj.email = root["email"].asString();
        if (root.isMember("age")) obj.age = root["age"].asInt();
        if (root.isMember("is_active")) obj.is_active = root["is_active"].asBool();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserRequest JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// UpdateUserRequest Protobuf序列化器实现
bool UpdateUserRequestProtobufSerializer::Serialize(const UpdateUserRequest& obj, std::string& output) {
    try {
        rpc_example::UpdateUserRequest pb_obj;
        pb_obj.set_user_id(obj.user_id);
        pb_obj.set_name(obj.name);
        pb_obj.set_email(obj.email);
        pb_obj.set_age(obj.age);
        pb_obj.set_is_active(obj.is_active);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserRequest Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UpdateUserRequestProtobufSerializer::Deserialize(const std::string& input, UpdateUserRequest& obj) {
    try {
        rpc_example::UpdateUserRequest pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.user_id = pb_obj.user_id();
        obj.name = pb_obj.name();
        obj.email = pb_obj.email();
        obj.age = pb_obj.age();
        obj.is_active = pb_obj.is_active();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserRequest Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// UpdateUserResponse JSON序列化器实现
bool UpdateUserResponseJsonSerializer::Serialize(const UpdateUserResponse& obj, std::string& output) {
    try {
        Json::Value root;
        root["user_id"] = obj.user_id;
        root["name"] = obj.name;
        root["email"] = obj.email;
        root["age"] = obj.age;
        root["is_active"] = obj.is_active;
        root["updated_at"] = obj.updated_at;
        root["status_message"] = obj.status_message;
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        output = Json::writeString(builder, root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserResponse JSON serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UpdateUserResponseJsonSerializer::Deserialize(const std::string& input, UpdateUserResponse& obj) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        std::istringstream iss(input);
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            std::cerr << "UpdateUserResponse JSON parse error: " << errors << std::endl;
            return false;
        }
        
        if (root.isMember("user_id")) obj.user_id = root["user_id"].asInt();
        if (root.isMember("name")) obj.name = root["name"].asString();
        if (root.isMember("email")) obj.email = root["email"].asString();
        if (root.isMember("age")) obj.age = root["age"].asInt();
        if (root.isMember("is_active")) obj.is_active = root["is_active"].asBool();
        if (root.isMember("updated_at")) obj.updated_at = root["updated_at"].asString();
        if (root.isMember("status_message")) obj.status_message = root["status_message"].asString();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserResponse JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

// UpdateUserResponse Protobuf序列化器实现
bool UpdateUserResponseProtobufSerializer::Serialize(const UpdateUserResponse& obj, std::string& output) {
    try {
        rpc_example::UpdateUserResponse pb_obj;
        pb_obj.set_user_id(obj.user_id);
        pb_obj.set_name(obj.name);
        pb_obj.set_email(obj.email);
        pb_obj.set_age(obj.age);
        pb_obj.set_is_active(obj.is_active);
        pb_obj.set_updated_at(obj.updated_at);
        pb_obj.set_status_message(obj.status_message);
        
        return pb_obj.SerializeToString(&output);
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserResponse Protobuf serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool UpdateUserResponseProtobufSerializer::Deserialize(const std::string& input, UpdateUserResponse& obj) {
    try {
        rpc_example::UpdateUserResponse pb_obj;
        if (!pb_obj.ParseFromString(input)) {
            return false;
        }
        
        obj.user_id = pb_obj.user_id();
        obj.name = pb_obj.name();
        obj.email = pb_obj.email();
        obj.age = pb_obj.age();
        obj.is_active = pb_obj.is_active();
        obj.updated_at = pb_obj.updated_at();
        obj.status_message = pb_obj.status_message();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UpdateUserResponse Protobuf deserialization error: " << e.what() << std::endl;
        return false;
    }
} 