#pragma once

#include "serialization_framework.h"
#include "network.h"
#include "type_registry.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <future>
#include <atomic>

// RPC状态码
enum class RpcStatus {
    SUCCESS = 0,
    METHOD_NOT_FOUND = 1,
    SERIALIZATION_ERROR = 2,
    NETWORK_ERROR = 3,
    TIMEOUT = 4,
    INVALID_REQUEST = 5,
    UNKNOWN_ERROR = 6
};

// RPC请求结构
struct RpcRequestData {
    std::string service_name;
    std::string method_name;
    std::string serializer_type;
    std::string request_data;
    int64_t request_id;
};

// RPC响应结构
struct RpcResponseData {
    int64_t request_id;
    RpcStatus status_code;
    std::string error_message;
    std::string response_data;
    std::string serializer_type; // 响应使用的序列化类型
};

// RPC方法处理器基类
class RpcMethodHandler {
public:
    virtual ~RpcMethodHandler() = default;
    virtual std::string HandleRequest(const std::string& request_data, 
                                     const std::string& serializer_type) = 0;
};

// 模板化的RPC方法处理器
template<typename RequestType, typename ResponseType>
class TypedRpcMethodHandler : public RpcMethodHandler {
public:
    using HandlerFunc = std::function<ResponseType(const RequestType&)>;
    
    TypedRpcMethodHandler(HandlerFunc handler) : handler_(handler) {}
    
    std::string HandleRequest(const std::string& request_data, 
                             const std::string& serializer_type) override {
        // 首先尝试获取类型特定的序列化器
        auto type_serializer_req = SerializerFactory::CreateTypeSerializer<RequestType>(serializer_type);
        auto type_serializer_resp = SerializerFactory::CreateTypeSerializer<ResponseType>(serializer_type);
        
        RequestType request;
        
        if (type_serializer_req) {
            // 使用类型特定的序列化器
            std::cout << "Server: Using type-specific " << serializer_type << " serializer for request" << std::endl;
            if (!type_serializer_req->Deserialize(request_data, request)) {
                throw std::runtime_error("Failed to deserialize request using type-specific serializer");
            }
        } else {
            // 回退到通用序列化器（反射）
            std::cout << "Server: Using generic " << serializer_type << " serializer for request" << std::endl;
            auto serializer = SerializerFactory::CreateSerializer(serializer_type);
            if (!serializer) {
                throw std::runtime_error("Unknown serializer type: " + serializer_type);
            }
            
            std::string type_name = TypeRegistry::Instance().GetTypeName<RequestType>();
            const auto* req_desc = ReflectionRegistry::Instance().GetDescriptor(type_name);
            if (!req_desc) {
                throw std::runtime_error("Request type not registered: " + type_name);
            }
            if (!serializer->Deserialize(request_data, &request, *req_desc)) {
                throw std::runtime_error("Failed to deserialize request");
            }
        }
        
        // 调用处理函数
        ResponseType response = handler_(request);
        
        // 序列化响应
        std::string response_data;
        
        if (type_serializer_resp) {
            // 使用类型特定的序列化器
            std::cout << "Server: Using type-specific " << serializer_type << " serializer for response" << std::endl;
            if (!type_serializer_resp->Serialize(response, response_data)) {
                throw std::runtime_error("Failed to serialize response using type-specific serializer");
            }
        } else {
            // 回退到通用序列化器（反射）
            std::cout << "Server: Using generic " << serializer_type << " serializer for response" << std::endl;
            auto serializer = SerializerFactory::CreateSerializer(serializer_type);
            if (!serializer) {
                throw std::runtime_error("Unknown serializer type: " + serializer_type);
            }
            
            std::string resp_type_name = TypeRegistry::Instance().GetTypeName<ResponseType>();
            const auto* resp_desc = ReflectionRegistry::Instance().GetDescriptor(resp_type_name);
            if (!resp_desc) {
                throw std::runtime_error("Response type not registered: " + resp_type_name);
            }
            if (!serializer->Serialize(&response, *resp_desc, response_data)) {
                throw std::runtime_error("Failed to serialize response");
            }
        }
        
        return response_data;
    }
    
private:
    HandlerFunc handler_;
};

// RPC服务器
class RpcServer {
public:
    RpcServer(EventLoop* loop, const std::string& ip, int port);
    ~RpcServer();
    
    void Start();
    void Stop();
    
    // 注册RPC方法
    template<typename RequestType, typename ResponseType>
    void RegisterMethod(const std::string& service_name, 
                       const std::string& method_name,
                       std::function<ResponseType(const RequestType&)> handler) {
        std::string full_name = service_name + "." + method_name;
        auto typed_handler = std::make_unique<TypedRpcMethodHandler<RequestType, ResponseType>>(handler);
        methods_[full_name] = std::move(typed_handler);
    }
    
private:
    EventLoop* loop_;
    std::unique_ptr<TcpServer> server_;
    std::unordered_map<std::string, std::unique_ptr<RpcMethodHandler>> methods_;
    
    void HandleConnection(std::shared_ptr<Connection> conn);
    void HandleMessage(std::shared_ptr<Connection> conn, const std::string& message);
    
    RpcRequestData ParseRequest(const std::string& message);
    std::string SerializeResponse(const RpcResponseData& response, const std::string& serializer_type = "json");
};

// RPC客户端
class RpcClient {
public:
    RpcClient(EventLoop* loop);
    ~RpcClient();
    
    void Connect(const std::string& ip, int port);
    void Disconnect();
    
    // 同步RPC调用
    template<typename RequestType, typename ResponseType>
    ResponseType Call(const std::string& service_name, 
                     const std::string& method_name,
                     const RequestType& request,
                     const std::string& serializer_type = "json") {
        // 序列化请求
        std::string request_data;
        
        // 首先尝试使用类型特定的序列化器
        auto type_serializer = SerializerFactory::CreateTypeSerializer<RequestType>(serializer_type);
        if (type_serializer) {
            std::cout << "Client: Using type-specific " << serializer_type << " serializer for request" << std::endl;
            if (!type_serializer->Serialize(request, request_data)) {
                throw std::runtime_error("Failed to serialize request using type-specific serializer");
            }
        } else {
            // 回退到通用序列化器（反射）
            std::cout << "Client: Using generic " << serializer_type << " serializer for request" << std::endl;
            auto serializer = SerializerFactory::CreateSerializer(serializer_type);
            if (!serializer) {
                throw std::runtime_error("Unknown serializer type: " + serializer_type);
            }
            
            std::string type_name = TypeRegistry::Instance().GetTypeName<RequestType>();
            const auto* req_desc = ReflectionRegistry::Instance().GetDescriptor(type_name);
            if (!req_desc) {
                throw std::runtime_error("Request type not registered: " + type_name);
            }
            if (!serializer->Serialize(&request, *req_desc, request_data)) {
                throw std::runtime_error("Failed to serialize request");
            }
        }
        
        // 创建RPC请求
        RpcRequestData rpc_request;
        rpc_request.service_name = service_name;
        rpc_request.method_name = method_name;
        rpc_request.serializer_type = serializer_type;
        rpc_request.request_data = request_data;
        rpc_request.request_id = GenerateRequestId();
        
        // 发送请求并等待响应
        auto future = SendRequest(rpc_request);
        RpcResponseData rpc_response = future.get();
        
        if (rpc_response.status_code != RpcStatus::SUCCESS) {
            throw std::runtime_error("RPC call failed: " + rpc_response.error_message);
        }
        
        // 反序列化响应
        std::string response_serializer_type = rpc_response.serializer_type.empty() ? serializer_type : rpc_response.serializer_type;
        ResponseType response;
        
        // 首先尝试使用类型特定的序列化器
        auto response_type_serializer = SerializerFactory::CreateTypeSerializer<ResponseType>(response_serializer_type);
        if (response_type_serializer) {
            std::cout << "Client: Using type-specific " << response_serializer_type << " serializer for response" << std::endl;
            if (!response_type_serializer->Deserialize(rpc_response.response_data, response)) {
                throw std::runtime_error("Failed to deserialize response using type-specific serializer");
            }
        } else {
            // 回退到通用序列化器（反射）
            std::cout << "Client: Using generic " << response_serializer_type << " serializer for response" << std::endl;
            auto response_serializer = SerializerFactory::CreateSerializer(response_serializer_type);
            if (!response_serializer) {
                throw std::runtime_error("Unknown response serializer type: " + response_serializer_type);
            }
            
            std::string resp_type_name = TypeRegistry::Instance().GetTypeName<ResponseType>();
            const auto* resp_desc = ReflectionRegistry::Instance().GetDescriptor(resp_type_name);
            if (!resp_desc) {
                throw std::runtime_error("Response type not registered: " + resp_type_name);
            }
            if (!response_serializer->Deserialize(rpc_response.response_data, &response, *resp_desc)) {
                throw std::runtime_error("Failed to deserialize response");
            }
        }
        
        return response;
    }
    
private:
    EventLoop* loop_;
    std::unique_ptr<TcpClient> client_;
    std::atomic<int64_t> request_id_counter_;
    
    std::mutex pending_requests_mutex_;
    std::unordered_map<int64_t, std::promise<RpcResponseData>> pending_requests_;
    
    void HandleConnection(std::shared_ptr<Connection> conn);
    void HandleMessage(std::shared_ptr<Connection> conn, const std::string& message);
    
    int64_t GenerateRequestId();
    std::future<RpcResponseData> SendRequest(const RpcRequestData& request);
    
    std::string SerializeRequest(const RpcRequestData& request);
    RpcResponseData ParseResponse(const std::string& message);
};

// 简单的消息协议：4字节长度 + 消息内容
class MessageCodec {
public:
    static std::string Encode(const std::string& message);
    static std::vector<std::string> Decode(std::string& buffer);
    
private:
    static const size_t kHeaderSize = 4;
}; 