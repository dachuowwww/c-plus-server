#pragma once

#include "serialization_framework.h"
#include <sstream>
#include <json/json.h>

class JsonSerializer : public ISerializer {
public:
    bool Serialize(const void* obj, const MessageDescriptor& desc, std::string& output) override {
        try {
            Json::Value root;
            
            for (const auto& field : desc.fields) {
                std::any value = field.getter(obj);
                
                switch (field.type) {
                    case FieldType::INT32:
                        root[field.name] = std::any_cast<int32_t>(value);
                        break;
                    case FieldType::INT64:
                        root[field.name] = static_cast<Json::Int64>(std::any_cast<int64_t>(value));
                        break;
                    case FieldType::UINT32:
                        root[field.name] = std::any_cast<uint32_t>(value);
                        break;
                    case FieldType::UINT64:
                        root[field.name] = static_cast<Json::UInt64>(std::any_cast<uint64_t>(value));
                        break;
                    case FieldType::FLOAT:
                        root[field.name] = std::any_cast<float>(value);
                        break;
                    case FieldType::DOUBLE:
                        root[field.name] = std::any_cast<double>(value);
                        break;
                    case FieldType::STRING:
                        root[field.name] = std::any_cast<std::string>(value);
                        break;
                    case FieldType::BOOL:
                        root[field.name] = std::any_cast<bool>(value);
                        break;
                    default:
                        std::cerr << "Unsupported field type for JSON serialization" << std::endl;
                        return false;
                }
            }
            
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";  // 紧凑格式
            output = Json::writeString(builder, root);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "JSON serialization error: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool Deserialize(const std::string& input, void* obj, const MessageDescriptor& desc) override {
        try {
            Json::Value root;
            Json::CharReaderBuilder builder;
            std::string errors;
            
            std::istringstream iss(input);
            // std::cout << "input: " << input << std::endl;
            if (!Json::parseFromStream(builder, iss, &root, &errors)) {
                std::cerr << "JSON parse error: " << errors << std::endl;
                return false;
            }
            
            for (const auto& field : desc.fields) {
                if (!root.isMember(field.name)) {
                    continue; // 跳过不存在的字段
                }
                
                const Json::Value& json_value = root[field.name];
                std::any value;
                
                switch (field.type) {
                    case FieldType::INT32:
                        value = json_value.asInt();
                        break;
                    case FieldType::INT64:
                        value = json_value.asInt64();
                        break;
                    case FieldType::UINT32:
                        value = json_value.asUInt();
                        break;
                    case FieldType::UINT64:
                        value = json_value.asUInt64();
                        break;
                    case FieldType::FLOAT:
                        value = json_value.asFloat();
                        break;
                    case FieldType::DOUBLE:
                        value = json_value.asDouble();
                        break;
                    case FieldType::STRING:
                        value = json_value.asString();
                        break;
                    case FieldType::BOOL:
                        value = json_value.asBool();
                        break;
                    default:
                        std::cerr << "Unsupported field type for JSON deserialization" << std::endl;
                        continue;
                }
                
                field.setter(obj, value);
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "JSON deserialization error: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::string GetName() const override {
        return "json";
    }
};

 