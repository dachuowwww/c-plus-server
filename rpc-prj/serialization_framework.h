#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <any>
#include <iostream>

// 字段类型枚举
enum class FieldType {
    INT32,
    INT64,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,
    BOOL,
    BYTES,
    MESSAGE
};

// 字段描述符
struct FieldDescriptor {
    std::string name;
    FieldType type;
    int field_number;
    
    // 通用的getter和setter，用于反射
    std::function<std::any(const void*)> getter;  // 从对象获取字段值
    std::function<void(void*, const std::any&)> setter;  // 设置字段值到对象
    
    FieldDescriptor() = default;
    FieldDescriptor(const std::string& n, FieldType t, int num) 
        : name(n), type(t), field_number(num) {}
};

// 消息描述符
struct MessageDescriptor {
    std::string message_name;
    std::vector<FieldDescriptor> fields;
    
    void AddField(const FieldDescriptor& field) {
        fields.push_back(field);
    }
};

// 序列化器接口
class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual bool Serialize(const void* obj, const MessageDescriptor& desc, std::string& output) = 0;
    virtual bool Deserialize(const std::string& input, void* obj, const MessageDescriptor& desc) = 0;
    virtual std::string GetName() const = 0;
};

// 类型特定的序列化器接口
template<typename T>
class ITypeSerializer {
public:
    virtual ~ITypeSerializer() = default;
    virtual bool Serialize(const T& obj, std::string& output) = 0;
    virtual bool Deserialize(const std::string& input, T& obj) = 0;
    virtual std::string GetName() const = 0;
};

// 序列化器注册表
class SerializerRegistry {
public:
    static SerializerRegistry& Instance() {
        static SerializerRegistry instance;
        return instance;
    }
    
    // 注册类型特定的序列化器
    template<typename T>
    void RegisterTypeSerializer(const std::string& serializer_name, 
                               std::unique_ptr<ITypeSerializer<T>> serializer) {
        std::string key = GetTypeSerializerKey<T>(serializer_name);
        type_serializers_[key] = std::make_unique<TypeSerializerWrapper<T>>(std::move(serializer));
    }
    
    // 获取类型特定的序列化器
    template<typename T>
    ITypeSerializer<T>* GetTypeSerializer(const std::string& serializer_name) {
        std::string key = GetTypeSerializerKey<T>(serializer_name);
        auto it = type_serializers_.find(key);
        if (it != type_serializers_.end()) {
            auto* wrapper = static_cast<TypeSerializerWrapper<T>*>(it->second.get());
            return wrapper->GetSerializer();
        }
        return nullptr;
    }
    
    // 注册通用序列化器（用于反射）
    void RegisterSerializer(const std::string& name, std::unique_ptr<ISerializer> serializer) {
        serializers_[name] = std::move(serializer);
    }
    
    // 获取通用序列化器
    ISerializer* GetSerializer(const std::string& name) {
        auto it = serializers_.find(name);
        return (it != serializers_.end()) ? it->second.get() : nullptr;
    }
    
private:
    template<typename T>
    std::string GetTypeSerializerKey(const std::string& serializer_name) {
        return std::string(typeid(T).name()) + "_" + serializer_name;
    }
    
    // 类型擦除的包装器
    class ITypeSerializerWrapper {
    public:
        virtual ~ITypeSerializerWrapper() = default;
    };
    
    template<typename T>
    class TypeSerializerWrapper : public ITypeSerializerWrapper {
    public:
        TypeSerializerWrapper(std::unique_ptr<ITypeSerializer<T>> serializer) 
            : serializer_(std::move(serializer)) {}
        
        ITypeSerializer<T>* GetSerializer() { return serializer_.get(); }
        
    private:
        std::unique_ptr<ITypeSerializer<T>> serializer_;
    };
    
    std::unordered_map<std::string, std::unique_ptr<ISerializer>> serializers_;
    std::unordered_map<std::string, std::unique_ptr<ITypeSerializerWrapper>> type_serializers_;
};

// 反射注册表
class ReflectionRegistry {
public:
    static ReflectionRegistry& Instance() {
        static ReflectionRegistry instance;
        return instance;
    }
    
    void RegisterMessage(const std::string& name, const MessageDescriptor& desc) {
        descriptors_[name] = desc;
    }
    
    const MessageDescriptor* GetDescriptor(const std::string& name) const {
        auto it = descriptors_.find(name);
        return (it != descriptors_.end()) ? &it->second : nullptr;
    }
    
private:
    std::unordered_map<std::string, MessageDescriptor> descriptors_;
};

// 序列化器工厂
class SerializerFactory {
public:
    // 创建类型特定的序列化器
    template<typename T>
    static ITypeSerializer<T>* CreateTypeSerializer(const std::string& serializer_type) {
        return SerializerRegistry::Instance().GetTypeSerializer<T>(serializer_type);
    }
    
    // 创建通用序列化器（用于反射）
    static ISerializer* CreateSerializer(const std::string& serializer_type) {
        return SerializerRegistry::Instance().GetSerializer(serializer_type);
    }
};

// 辅助函数：为类添加字段到描述符
template<typename ClassType, typename FieldType>
void AddFieldToDescriptor(MessageDescriptor& desc, const std::string& field_name, 
                         int field_number, ::FieldType field_type, 
                         FieldType ClassType::* field_ptr) {
    FieldDescriptor field(field_name, field_type, field_number);
    
    // 设置getter
    field.getter = [field_ptr](const void* obj) -> std::any {
        const ClassType* typed_obj = static_cast<const ClassType*>(obj);
        return typed_obj->*field_ptr;
    };
    
    // 设置setter
    field.setter = [field_ptr](void* obj, const std::any& value) {
        ClassType* typed_obj = static_cast<ClassType*>(obj);
        typed_obj->*field_ptr = std::any_cast<FieldType>(value);
    };
    
    desc.AddField(field);
}

// 宏定义：简化序列化器注册
#define REGISTER_TYPE_SERIALIZER(Type, SerializerName, SerializerClass) \
    SerializerRegistry::Instance().RegisterTypeSerializer<Type>( \
        SerializerName, std::make_unique<SerializerClass>())

#define REGISTER_SERIALIZER(SerializerName, SerializerClass) \
    SerializerRegistry::Instance().RegisterSerializer( \
        SerializerName, std::make_unique<SerializerClass>()) 
