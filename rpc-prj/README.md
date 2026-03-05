# 高性能RPC框架 - 可插拔序列化系统

这是一个基于C++17实现的高性能RPC框架，采用可插拔序列化架构，支持JSON、Protobuf等多种序列化格式。框架使用Reactor模式 + epoll实现高并发网络处理，支持异步RPC调用。

## 🚀 核心特性

- **双重序列化架构**: 类型特定序列化器 + 通用反射序列化器
- **高性能网络**: Reactor模式 + epoll事件循环
- **异步RPC**: Promise/Future模式实现非阻塞调用
- **类型安全**: 编译时类型检查 + 运行时反射
- **可扩展**: 插件化架构，轻松添加新的序列化格式
- **协议分层**: 清晰的协议层次设计

## 📁 项目结构

```
├── serialization_framework.h        # 序列化框架核心接口
├── json_serializer.h               # JSON通用序列化器
├── type_registry.h                 # 类型注册表
├── network.h/cpp                   # 网络层 (Reactor + epoll)
├── rpc_framework.h/cpp             # RPC框架核心
├── user_types.h                    # 用户定义的请求/响应类型
├── user_type_serializers.h/cpp     # 类型特定序列化器
├── user.proto                     # Protobuf消息定义
├── main.cpp       # 类型特定序列化器示例
├── CMakeLists.txt                  # 构建配置
└── README.md                       # 项目说明
```

## 🛠️ 编译环境要求

### 系统要求
- Linux (支持epoll)
- GCC 7.0+ 或 Clang 6.0+ (支持C++17)
- CMake 3.10+

### 依赖库
- **Protobuf 3.0+** (可选，用于protobuf序列化)
- **JsonCpp** (用于JSON序列化)

### 安装依赖 (Ubuntu/Debian)

```bash
# 安装基础工具
sudo apt update
sudo apt install build-essential cmake pkg-config

# 安装JsonCpp
sudo apt install libjsoncpp-dev

# 安装Protobuf (可选)
sudo apt install libprotobuf-dev protobuf-compiler
```
 

## 🔨 编译步骤

### 1. 创建构建目录
```bash
mkdir build
cd build
```

### 2. 配置和编译
```bash
# 配置项目
cmake ..

# 编译所有示例
make
```

### 3. 编译输出
编译成功后，在`build`目录下会生成以下可执行文件：
- `rpc` - 类型特定序列化器示例

## 🧪 运行测试示例

 
这个示例展示了完整的RPC功能，包括JSON和Protobuf两种序列化方式。

```bash
# 终端1: 启动服务器
cd build
./rpc server

# 终端2: 运行客户端
cd build
./rpc client
```

**功能演示:**
- GetUser/CreateUser/UpdateUser 三种RPC方法
- JSON和Protobuf两种序列化格式
- 不同的请求和响应类型
- 类型特定序列化器的使用

**预期输出:**
```
=== 新的请求/响应类型演示 ===

1. GetUser (JSON):
GetUser响应: 
  用户ID: 123
  姓名: 张三
  邮箱: zhangsan@example.com
  年龄: 28
  活跃状态: 是
  状态消息: 用户信息获取成功

2. CreateUser (JSON):
CreateUser响应: 
  新用户ID: 12345
  姓名: 李四
  邮箱: lisi@example.com
  创建时间: 2024-01-01 12:00:00
  状态消息: 用户创建成功

... (更多测试结果)
```
  

### 2. 注册序列化器

```cpp
// 注册通用序列化器
REGISTER_SERIALIZER("json", JsonSerializer);

// 注册类型特定序列化器
REGISTER_TYPE_SERIALIZER(GetUserRequest, "json", GetUserRequestJsonSerializer);
```

### 3. 实现RPC服务

```cpp
class UserServiceImpl {
public:
    GetUserResponse GetUser(const GetUserRequest& request) {
        // 业务逻辑实现
        GetUserResponse response;
        response.user_id = request.user_id;
        response.name = "张三";
        return response;
    }
};

// 注册RPC方法
server.RegisterMethod<GetUserRequest, GetUserResponse>("UserService", "GetUser",
    [&service_impl](const GetUserRequest& request) {
        return service_impl.GetUser(request);
    });
```

### 4. 调用RPC服务

```cpp
GetUserRequest request;
request.user_id = 123;

GetUserResponse response = client.Call<GetUserRequest, GetUserResponse>(
    "UserService", "GetUser", request, "json");
```

## 🏗️ 架构设计

### 协议层次
```
应用层 (UserServiceImpl)
    ↕
RPC框架层 (RpcServer/RpcClient)
    ↕
业务协议层 (JSON/Protobuf序列化)
    ↕
RPC协议层 (4字节长度头 + JSON消息)
    ↕
传输层 (TCP + Reactor + epoll)
```

### 序列化策略
```
类型特定序列化器 (高性能)
    ↕ (优先级高)
通用序列化器 (反射机制)
```

## 🐛 故障排除
 

### 运行时错误

1. **端口被占用**
   ```bash
   # 检查端口占用
   netstat -tlnp | grep 10005
   # 杀死占用进程
   sudo kill -9 <pid>
   ```

2. **连接失败**
   - 确保服务器已启动
   - 检查防火墙设置
   - 确认端口号正确

## 📚 扩展开发

### 添加新的序列化格式

```cpp
// 1. 实现序列化器接口
class MessagePackSerializer : public ISerializer {
public:
    bool Serialize(const void* obj, const MessageDescriptor& desc, std::string& output) override;
    bool Deserialize(const std::string& input, void* obj, const MessageDescriptor& desc) override;
    std::string GetName() const override { return "messagepack"; }
};

// 2. 注册序列化器
REGISTER_SERIALIZER("messagepack", MessagePackSerializer);

// 3. 使用新格式
auto response = client.Call<Request, Response>("Service", "Method", request, "messagepack");
```
 

## 🎯 面试要点

1. **系统架构设计** - 分层架构、插件化设计
2. **设计模式应用** - 工厂模式、策略模式、观察者模式
3. **C++高级特性** - 模板、RAII、智能指针、类型擦除
4. **网络编程** - epoll、Reactor模式、异步编程
5. **序列化技术** - JSON、Protobuf、反射机制
6. **性能优化** - 零拷贝、内存池、批量处理

 