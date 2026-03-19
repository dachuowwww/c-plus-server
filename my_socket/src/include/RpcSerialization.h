#pragma once
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

// 字段类型
enum class RpcFieldType {
  INT32,
  INT64,
  STRING,
  BOOL,
  BYTES,
};

// 字段描述符
struct RpcFieldDescriptor {
  std::string name;
  RpcFieldType type = RpcFieldType::STRING;
  std::function<std::any(const void *)> getter;          // 读
  std::function<void(void *, const std::any &)> setter;  // 写
};

// 消息描述符
struct RpcMessageDescriptor {
  std::string message_name;
  std::vector<RpcFieldDescriptor> fields;
};

// 序列化器接口
class ISerializer {
 public:
  virtual ~ISerializer() = default;
  virtual bool Serialize(const void *obj, const RpcMessageDescriptor &desc, std::string *output) = 0;
  virtual bool Deserialize(const std::string &input, void *obj, const RpcMessageDescriptor &desc) = 0;
  virtual std::string GetName() const = 0;
};

// 类型特定的序列化器接口
template <typename T>  // 信息封装类型特定的序列化器接口
class ITypeSerializer {
 public:
  virtual ~ITypeSerializer() = default;
  virtual bool Serialize(const T &obj, std::string *output) = 0;
  virtual bool Deserialize(const std::string &input, T *obj) = 0;
  virtual std::string GetName() const = 0;
};

// 序列化器注册表
class SerializerRegistry {
 public:
  static SerializerRegistry &Instance() {
    static SerializerRegistry instance;
    return instance;
  }

  void RegisterSerializer(const std::string &name, std::unique_ptr<ISerializer> serializer) {
    serializers_[name] = std::move(serializer);
  }

  ISerializer *GetSerializer(const std::string &name) const {
    auto it = serializers_.find(name);
    if (it == serializers_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  template <typename T>
  void RegisterTypeSerializer(const std::string &serializer_name, std::unique_ptr<ITypeSerializer<T>> serializer) {
    std::string key = TypeSerializerKey<T>(serializer_name);
    type_serializers_[key] = std::make_unique<TypeSerializerWrapper<T>>(std::move(serializer));
  }

  template <typename T>
  ITypeSerializer<T> *GetTypeSerializer(const std::string &serializer_name) const {
    std::string key = TypeSerializerKey<T>(serializer_name);
    auto it = type_serializers_.find(key);
    if (it == type_serializers_.end()) {
      return nullptr;
    }
    auto *wrapper = dynamic_cast<TypeSerializerWrapper<T> *>(it->second.get());
    if (!wrapper) {
      return nullptr;
    }
    return wrapper->serializer.get();
  }

 private:
  class ITypeSerializerWrapper {
   public:
    virtual ~ITypeSerializerWrapper() = default;
  };

  template <typename T>
  class TypeSerializerWrapper : public ITypeSerializerWrapper {
   public:
    explicit TypeSerializerWrapper(std::unique_ptr<ITypeSerializer<T>> s) : serializer(std::move(s)) {}
    std::unique_ptr<ITypeSerializer<T>> serializer;
  };

  template <typename T>
  static std::string TypeSerializerKey(const std::string &serializer_name) {
    return std::string(typeid(T).name()) + "_" + serializer_name;
  }

  std::unordered_map<std::string, std::unique_ptr<ISerializer>> serializers_;
  std::unordered_map<std::string, std::unique_ptr<ITypeSerializerWrapper>> type_serializers_;
};

// 反射注册表
class ReflectionRegistry {
 public:
  static ReflectionRegistry &Instance() {
    static ReflectionRegistry instance;
    return instance;
  }

  void RegisterMessage(const std::string &name, RpcMessageDescriptor desc) { messages_[name] = std::move(desc); }

  const RpcMessageDescriptor *GetDescriptor(const std::string &name) const {
    auto it = messages_.find(name);
    if (it == messages_.end()) {
      return nullptr;
    }
    return &it->second;
  }

 private:
  std::unordered_map<std::string, RpcMessageDescriptor> messages_;
};

// RPC类型注册表，用于将C++类型映射到字符串类型名，便于在序列化时通过反射获取描述符
class RpcTypeRegistry {
 public:
  static RpcTypeRegistry &Instance() {
    static RpcTypeRegistry instance;
    return instance;
  }

  template <typename T>
  void RegisterType(const std::string &name) {
    type_names_[typeid(T).name()] = name;
  }

  template <typename T>
  std::string GetTypeName() const {
    auto it = type_names_.find(typeid(T).name());
    if (it == type_names_.end()) {
      return typeid(T).name();
    }
    return it->second;
  }

 private:
  std::unordered_map<std::string, std::string> type_names_;
};

template <typename ClassType, typename FieldTypeT>  // 类类型和类中的字段类型，在注册函数时转换
void AddFieldToDescriptor(RpcMessageDescriptor *desc, const std::string &field_name, RpcFieldType field_type,
                          FieldTypeT ClassType::*field_ptr) {
  RpcFieldDescriptor field;
  field.name = field_name;
  field.type = field_type;
  field.getter = [field_ptr](const void *obj) -> std::any {
    const auto *typed = static_cast<const ClassType *>(obj);
    return typed->*field_ptr;
  };
  field.setter = [field_ptr](void *obj, const std::any &value) {
    auto *typed = static_cast<ClassType *>(obj);
    typed->*field_ptr = std::any_cast<FieldTypeT>(value);
  };
  desc->fields.push_back(std::move(field));
}

#define REGISTER_RPC_SERIALIZER(Name, SerializerClass) \
  SerializerRegistry::Instance().RegisterSerializer(Name, std::make_unique<SerializerClass>())

#define REGISTER_RPC_TYPE_SERIALIZER(Type, Name, SerializerClass) \
  SerializerRegistry::Instance().RegisterTypeSerializer<Type>(Name, std::make_unique<SerializerClass>())

#define REGISTER_RPC_TYPE(Type, Name) RpcTypeRegistry::Instance().RegisterType<Type>(Name)
