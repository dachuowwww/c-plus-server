#include "serialization_framework.h"
#include "json_serializer.h"

// 自动注册通用序列化器
class SerializerRegistrar {
public:
    SerializerRegistrar() {
        // 注册通用JSON序列化器（用于反射）
        REGISTER_SERIALIZER("json", JsonSerializer);
        
        std::cout << "Registered generic JSON serializer" << std::endl;
    }
};

// 全局注册器实例
static SerializerRegistrar g_registrar; 