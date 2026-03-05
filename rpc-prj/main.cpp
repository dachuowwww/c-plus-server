#include "rpc_framework.h"
#include "user_types.h"
#include "user_type_serializers.h"
#include "type_registry.h"
#include "json_serializer.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// 用户服务实现
class UserServiceImpl {
public:
    GetUserResponse GetUser(const GetUserRequest& request) {
        std::cout << "GetUser called for user_id: " << request.user_id << std::endl;
        
        GetUserResponse response;
        response.user_id = request.user_id;
        response.name = "张三";
        response.email = "zhangsan@example.com";
        response.age = 28;
        response.is_active = true;
        response.status_message = "用户信息获取成功";
        
        return response;
    }
    
    CreateUserResponse CreateUser(const CreateUserRequest& request) {
        std::cout << "CreateUser called: " << request.name << std::endl;
        
        CreateUserResponse response;
        response.user_id = 12345; // 模拟分配新ID
        response.name = request.name;
        response.email = request.email;
        response.age = request.age;
        response.is_active = request.is_active;
        
        // 生成创建时间
        auto now = std::time(nullptr);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
        response.created_at = oss.str();
        response.status_message = "用户创建成功";
        
        return response;
    }
    
    UpdateUserResponse UpdateUser(const UpdateUserRequest& request) {
        std::cout << "UpdateUser called for user_id: " << request.user_id << std::endl;
        
        UpdateUserResponse response;
        response.user_id = request.user_id;
        response.name = request.name;
        response.email = request.email;
        response.age = request.age;
        response.is_active = request.is_active;
        
        // 生成更新时间
        auto now = std::time(nullptr);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
        response.updated_at = oss.str();
        response.status_message = "用户信息更新成功";
        
        return response;
    }
};

void RunServer() {
    // 注册通用序列化器
    REGISTER_SERIALIZER("json", JsonSerializer);
    std::cout << "Registered JSON serializer" << std::endl;
    
    // 注册类型特定的序列化器
    REGISTER_TYPE_SERIALIZER(GetUserRequest, "json", GetUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(GetUserResponse, "json", GetUserResponseJsonSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserRequest, "json", CreateUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserResponse, "json", CreateUserResponseJsonSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserRequest, "json", UpdateUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserResponse, "json", UpdateUserResponseJsonSerializer);
    
    REGISTER_TYPE_SERIALIZER(GetUserRequest, "protobuf", GetUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(GetUserResponse, "protobuf", GetUserResponseProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserRequest, "protobuf", CreateUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserResponse, "protobuf", CreateUserResponseProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserRequest,"protobuf",UpdateUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserResponse,"protobuf",UpdateUserResponseProtobufSerializer);
    
    std::cout << "Registered type-specific serializers" << std::endl;
    
    // 注册类型和反射信息
    REGISTER_TYPE(GetUserRequest);
    REGISTER_TYPE(GetUserResponse);
    REGISTER_TYPE(CreateUserRequest);
    REGISTER_TYPE(CreateUserResponse);
    REGISTER_TYPE(UpdateUserRequest);
    REGISTER_TYPE(UpdateUserResponse);
    
    GetUserRequest::RegisterReflection();
    GetUserResponse::RegisterReflection();
    CreateUserRequest::RegisterReflection();
    CreateUserResponse::RegisterReflection();
    UpdateUserRequest::RegisterReflection();
    UpdateUserResponse::RegisterReflection();
    
    std::cout << "Registered all request/response types" << std::endl;
    
    EventLoop loop;
    RpcServer server(&loop, "127.0.0.1", 10005);
    
    // 创建服务实现
    UserServiceImpl service_impl;
    
    // 注册RPC方法 - 使用不同的请求和响应类型
    server.RegisterMethod<GetUserRequest, GetUserResponse>("UserService", "GetUser",
        [&service_impl](const GetUserRequest& request) {
            return service_impl.GetUser(request);
        });
    
    server.RegisterMethod<CreateUserRequest, CreateUserResponse>("UserService", "CreateUser",
        [&service_impl](const CreateUserRequest& request) {
            return service_impl.CreateUser(request);
        });
    
    server.RegisterMethod<UpdateUserRequest, UpdateUserResponse>("UserService", "UpdateUser",
        [&service_impl](const UpdateUserRequest& request) {
            return service_impl.UpdateUser(request);
        });
    
    std::cout << "Starting New Types RPC server on port 10005..." << std::endl;
    server.Start();
    
    // 运行事件循环
    loop.Run();
}

void RunClient() {
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 注册通用序列化器
    REGISTER_SERIALIZER("json", JsonSerializer);
    std::cout << "Registered JSON serializer" << std::endl;
    
    // 注册类型特定的序列化器
    REGISTER_TYPE_SERIALIZER(GetUserRequest, "json", GetUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(GetUserResponse, "json", GetUserResponseJsonSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserRequest, "json", CreateUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserResponse, "json", CreateUserResponseJsonSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserRequest, "json", UpdateUserRequestJsonSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserResponse, "json", UpdateUserResponseJsonSerializer);
    
    REGISTER_TYPE_SERIALIZER(GetUserRequest, "protobuf", GetUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(GetUserResponse, "protobuf", GetUserResponseProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserRequest, "protobuf", CreateUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(CreateUserResponse, "protobuf", CreateUserResponseProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserRequest, "protobuf", UpdateUserRequestProtobufSerializer);
    REGISTER_TYPE_SERIALIZER(UpdateUserResponse, "protobuf", UpdateUserResponseProtobufSerializer);
    
    std::cout << "Registered type-specific serializers" << std::endl;
    
    // 注册类型和反射信息
    REGISTER_TYPE(GetUserRequest);
    REGISTER_TYPE(GetUserResponse);
    REGISTER_TYPE(CreateUserRequest);
    REGISTER_TYPE(CreateUserResponse);
    REGISTER_TYPE(UpdateUserRequest);
    REGISTER_TYPE(UpdateUserResponse);
    
    GetUserRequest::RegisterReflection();
    GetUserResponse::RegisterReflection();
    CreateUserRequest::RegisterReflection();
    CreateUserResponse::RegisterReflection();
    UpdateUserRequest::RegisterReflection();
    UpdateUserResponse::RegisterReflection();
    
    std::cout << "Registered all request/response types" << std::endl;
    
    EventLoop loop;
    RpcClient client(&loop);
    
    // 在单独线程中运行事件循环
    std::thread loop_thread([&loop]() {
        loop.Run();
    });
    
    try {
        // 连接到服务器
        client.Connect("127.0.0.1", 10005);
        
        // 等待连接建立
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "\n=== 新的请求/响应类型演示 ===" << std::endl;
        
        // 测试1: GetUser - 使用JSON序列化
        std::cout << "\n1. GetUser (JSON):" << std::endl;
        try {
            GetUserRequest get_request;
            get_request.user_id = 123;
            
            GetUserResponse get_response = client.Call<GetUserRequest, GetUserResponse>(
                "UserService", "GetUser", get_request, "json");
            
            std::cout << "GetUser响应: " << std::endl;
            std::cout << "  用户ID: " << get_response.user_id << std::endl;
            std::cout << "  姓名: " << get_response.name << std::endl;
            std::cout << "  邮箱: " << get_response.email << std::endl;
            std::cout << "  年龄: " << get_response.age << std::endl;
            std::cout << "  活跃状态: " << (get_response.is_active ? "是" : "否") << std::endl;
            std::cout << "  状态消息: " << get_response.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "GetUser调用失败: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 测试2: CreateUser - 使用JSON序列化
        std::cout << "\n2. CreateUser (JSON):" << std::endl;
        try {
            CreateUserRequest create_request;
            create_request.name = "李四";
            create_request.email = "lisi@example.com";
            create_request.age = 25;
            create_request.is_active = true;
            
            CreateUserResponse create_response = client.Call<CreateUserRequest, CreateUserResponse>(
                "UserService", "CreateUser", create_request, "json");
            
            std::cout << "CreateUser响应: " << std::endl;
            std::cout << "  新用户ID: " << create_response.user_id << std::endl;
            std::cout << "  姓名: " << create_response.name << std::endl;
            std::cout << "  邮箱: " << create_response.email << std::endl;
            std::cout << "  创建时间: " << create_response.created_at << std::endl;
            std::cout << "  状态消息: " << create_response.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "CreateUser调用失败: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 测试3: UpdateUser - 使用JSON序列化
        std::cout << "\n3. UpdateUser (JSON):" << std::endl;
        try {
            UpdateUserRequest update_request;
            update_request.user_id = 123;
            update_request.name = "张三（已更新）";
            update_request.email = "zhangsan_updated@example.com";
            update_request.age = 29;
            update_request.is_active = true;
            
            UpdateUserResponse update_response = client.Call<UpdateUserRequest, UpdateUserResponse>(
                "UserService", "UpdateUser", update_request, "json");
            
            std::cout << "UpdateUser响应: " << std::endl;
            std::cout << "  用户ID: " << update_response.user_id << std::endl;
            std::cout << "  姓名: " << update_response.name << std::endl;
            std::cout << "  邮箱: " << update_response.email << std::endl;
            std::cout << "  更新时间: " << update_response.updated_at << std::endl;
            std::cout << "  状态消息: " << update_response.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "UpdateUser调用失败: " << e.what() << std::endl;
        }
        
        // 测试4: GetUser - 使用Protobuf序列化
        std::cout << "\n4. GetUser (Protobuf):" << std::endl;
        try {
            GetUserRequest get_request_pb;
            get_request_pb.user_id = 456;
            
            GetUserResponse get_response_pb = client.Call<GetUserRequest, GetUserResponse>(
                "UserService", "GetUser", get_request_pb, "protobuf");
            
            std::cout << "GetUser响应 (Protobuf): " << std::endl;
            std::cout << "  用户ID: " << get_response_pb.user_id << std::endl;
            std::cout << "  姓名: " << get_response_pb.name << std::endl;
            std::cout << "  邮箱: " << get_response_pb.email << std::endl;
            std::cout << "  年龄: " << get_response_pb.age << std::endl;
            std::cout << "  活跃状态: " << (get_response_pb.is_active ? "是" : "否") << std::endl;
            std::cout << "  状态消息: " << get_response_pb.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "GetUser (Protobuf) 调用失败: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 测试5: CreateUser - 使用Protobuf序列化
        std::cout << "\n5. CreateUser (Protobuf):" << std::endl;
        try {
            CreateUserRequest create_request_pb;
            create_request_pb.name = "王五";
            create_request_pb.email = "wangwu@example.com";
            create_request_pb.age = 30;
            create_request_pb.is_active = true;
            
            CreateUserResponse create_response_pb = client.Call<CreateUserRequest, CreateUserResponse>(
                "UserService", "CreateUser", create_request_pb, "protobuf");
            
            std::cout << "CreateUser响应 (Protobuf): " << std::endl;
            std::cout << "  新用户ID: " << create_response_pb.user_id << std::endl;
            std::cout << "  姓名: " << create_response_pb.name << std::endl;
            std::cout << "  邮箱: " << create_response_pb.email << std::endl;
            std::cout << "  创建时间: " << create_response_pb.created_at << std::endl;
            std::cout << "  状态消息: " << create_response_pb.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "CreateUser (Protobuf) 调用失败: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 测试6: UpdateUser - 使用Protobuf序列化
        std::cout << "\n6. UpdateUser (Protobuf):" << std::endl;
        try {
            UpdateUserRequest update_request_pb;
            update_request_pb.user_id = 456;
            update_request_pb.name = "张三（Protobuf更新）";
            update_request_pb.email = "zhangsan_pb_updated@example.com";
            update_request_pb.age = 30;
            update_request_pb.is_active = true;
            
            UpdateUserResponse update_response_pb = client.Call<UpdateUserRequest, UpdateUserResponse>(
                "UserService", "UpdateUser", update_request_pb, "protobuf");
            
            std::cout << "UpdateUser响应 (Protobuf): " << std::endl;
            std::cout << "  用户ID: " << update_response_pb.user_id << std::endl;
            std::cout << "  姓名: " << update_response_pb.name << std::endl;
            std::cout << "  邮箱: " << update_response_pb.email << std::endl;
            std::cout << "  更新时间: " << update_response_pb.updated_at << std::endl;
            std::cout << "  状态消息: " << update_response_pb.status_message << std::endl;
        } catch (const std::exception& e) {
            std::cout << "UpdateUser (Protobuf) 调用失败: " << e.what() << std::endl;
        }

        std::cout << "\n=== 新类型演示完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "RPC调用错误: " << e.what() << std::endl;
    }
    
    // 停止事件循环
    loop.Stop();
    loop_thread.join();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " [server|client]" << std::endl;
        std::cout << "演示功能:" << std::endl;
        std::cout << "  - 使用专门的请求和响应类型" << std::endl;
        std::cout << "  - GetUserRequest -> GetUserResponse" << std::endl;
        std::cout << "  - CreateUserRequest -> CreateUserResponse" << std::endl;
        std::cout << "  - UpdateUserRequest -> UpdateUserResponse" << std::endl;
        std::cout << "  - 每种响应类型包含额外的状态信息" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "server") {
        RunServer();
    } else if (mode == "client") {
        RunClient();
    } else {
        std::cout << "无效的模式，请使用 'server' 或 'client'" << std::endl;
        return 1;
    }
    
    return 0;
} 