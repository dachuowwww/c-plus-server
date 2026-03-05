#pragma once

#include <string>
#include <unordered_map>
#include <typeinfo>
#include <cctype>

// 简化的类型注册系统
class TypeRegistry {
public:
    static TypeRegistry& Instance() {
        static TypeRegistry instance;
        return instance;
    }
    
    // 注册类型时同时记录类型ID到名称的映射
    template<typename T>
    void RegisterType(const std::string& name) {
        std::string type_id = typeid(T).name();
        type_id_to_name_[type_id] = name;
        type_names_[name] = name;
    }
    
    // 根据类型获取注册的名称
    template<typename T>
    std::string GetTypeName() const {
        std::string type_id = typeid(T).name();
        auto it = type_id_to_name_.find(type_id);
        if (it != type_id_to_name_.end()) {
            return it->second;
        }
        
        // 如果没有注册，返回默认的类型名（去掉编译器修饰）
        return DemangleTypeName(type_id);
    }
    
    bool HasType(const std::string& name) const {
        return type_names_.find(name) != type_names_.end();
    }
    
private:
    std::unordered_map<std::string, std::string> type_names_;           // 名称到名称的映射
    std::unordered_map<std::string, std::string> type_id_to_name_;      // 类型ID到名称的映射
    
    // 简化的类型名去修饰函数
    std::string DemangleTypeName(const std::string& mangled_name) const {
        // 简单的去修饰逻辑，提取类名
        std::string result = mangled_name;
        
        // 去掉数字前缀（GCC的修饰）
        size_t start = 0;
        while (start < result.length() && std::isdigit(result[start])) {
            start++;
        }
        if (start > 0 && start < result.length()) {
            result = result.substr(start);
        }
        
        // 查找最后一个::之后的部分（类名）
        size_t last_colon = result.find_last_of(':');
        if (last_colon != std::string::npos && last_colon + 1 < result.length()) {
            result = result.substr(last_colon + 1);
        }
        
        return result;
    }
};

// 宏定义简化类型注册
#define REGISTER_TYPE(Type) \
    TypeRegistry::Instance().RegisterType<Type>(#Type)

#define REGISTER_TYPE_WITH_NAME(Type, name) \
    TypeRegistry::Instance().RegisterType<Type>(name) 
