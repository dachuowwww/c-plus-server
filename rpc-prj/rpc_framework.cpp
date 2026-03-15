#include "rpc_framework.h"
#include <sstream>
#include <cstring>

// MessageCodec 实现
std::string MessageCodec::Encode(const std::string& message) { // 后面的消息体有多长。
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    std::string encoded;
    encoded.append(reinterpret_cast<const char*>(&length), kHeaderSize);
    encoded.append(message);
    return encoded;
}

std::vector<std::string> MessageCodec::Decode(std::string& buffer) { // 可能接受多条
    std::vector<std::string> messages;
    
    while (buffer.size() >= kHeaderSize) {
        uint32_t length;
        memcpy(&length, buffer.data(), kHeaderSize);
        length = ntohl(length);
        
        if (buffer.size() < kHeaderSize + length) {
            break; // 消息不完整
        }
        
        std::string message = buffer.substr(kHeaderSize, length);
        messages.push_back(message);
        
        buffer.erase(0, kHeaderSize + length);
    }
    
    return messages;
}

// RpcServer 实现
RpcServer::RpcServer(EventLoop* loop, const std::string& ip, int port)
    : loop_(loop) {
    server_ = std::make_unique<TcpServer>(loop, ip, port);
    server_->SetConnectionCallback([this](std::shared_ptr<Connection> conn) {
        HandleConnection(conn);
    });
    server_->SetMessageCallback([this](std::shared_ptr<Connection> conn, const std::string& message) {
        HandleMessage(conn, message);
    });
}

RpcServer::~RpcServer() {
    Stop();
}

void RpcServer::Start() {
    server_->Start();
}

void RpcServer::Stop() {
    server_->Stop();
}

void RpcServer::HandleConnection(std::shared_ptr<Connection> conn) {
    std::cout << "RPC client connected, fd: " << conn->GetFd() << std::endl;
}

void RpcServer::HandleMessage(std::shared_ptr<Connection> conn, const std::string& message) {  // 连接socket，消息内容read_buffer
    try {
        std::cout << "Server: Received raw message: " << message << std::endl;
        
        // 解码消息
        std::string buffer = message;
        auto messages = MessageCodec::Decode(buffer);
        
        for (const auto& decoded_message : messages) {
            std::cout << "Server: Decoded message: " << decoded_message << std::endl;
            
            // 解析RPC请求
            RpcRequestData request = ParseRequest(decoded_message);
            
            std::cout << "Server: Parsed request - Service: " << request.service_name 
                      << ", Method: " << request.method_name 
                      << ", Serializer: " << request.serializer_type 
                      << ", RequestID: " << request.request_id 
                      << ", RequestData: " << request.request_data << std::endl;
            
            // 检查解析结果
            if (request.service_name.empty() || request.method_name.empty()) {
                std::cout << "Server: Invalid request - missing service or method name" << std::endl;
                RpcResponseData error_response;
                error_response.request_id = request.request_id;
                error_response.status_code = RpcStatus::INVALID_REQUEST;
                error_response.error_message = "Invalid request format";
                
                std::string response_message = SerializeResponse(error_response);
                std::string encoded_response = MessageCodec::Encode(response_message);
                conn->Send(encoded_response);
                continue;
            }
            
            // 如果未指定序列化类型，默认使用JSON
            if (request.serializer_type.empty()) {
                request.serializer_type = "json";
                std::cout << "Server: No serializer type specified, defaulting to JSON" << std::endl;
            }
            
            // 查找方法处理器
            std::string full_method_name = request.service_name + "." + request.method_name;
            auto it = methods_.find(full_method_name);
            
            std::cout << "Server: Looking for method: " << full_method_name << std::endl;
            
            RpcResponseData response;
            response.request_id = request.request_id;
            
            if (it == methods_.end()) {
                std::cout << "Server: Method not found: " << full_method_name << std::endl;
                response.status_code = RpcStatus::METHOD_NOT_FOUND;
                response.error_message = "Method not found: " + full_method_name;
            } else {
                try {
                    std::cout << "Server: Calling method handler..." << std::endl;
                    // 调用方法处理器
                    response.response_data = it->second->HandleRequest(request.request_data, request.serializer_type);
                    response.status_code = RpcStatus::SUCCESS;
                    std::cout << "Server: Method call successful" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "Server: Method call failed: " << e.what() << std::endl;
                    response.status_code = RpcStatus::SERIALIZATION_ERROR;
                    response.error_message = e.what();
                }
            }
            
            // 发送响应（使用与请求相同的序列化格式）
            std::string response_message = SerializeResponse(response, request.serializer_type);
            std::cout << "Server: Sending response using " << request.serializer_type << " format: " << response_message << std::endl;
            std::string encoded_response = MessageCodec::Encode(response_message);
            conn->Send(encoded_response);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling RPC request: " << e.what() << std::endl;
    }
}

RpcRequestData RpcServer::ParseRequest(const std::string& message) {
    // 简单的JSON解析
    RpcRequestData request;
    
    // 直接在整个消息中查找字段
    std::string msg = message;
    
    // 解析 service_name
    size_t pos = msg.find("\"service_name\":\"");
    if (pos != std::string::npos) {
        pos += 16; // 跳过 "service_name":"
        size_t end = msg.find("\"", pos);
        if (end != std::string::npos) {
            request.service_name = msg.substr(pos, end - pos);
        }
    }
    
    // 解析 method_name
    pos = msg.find("\"method_name\":\"");
    if (pos != std::string::npos) {
        pos += 15; // 跳过 "method_name":"
        size_t end = msg.find("\"", pos);
        if (end != std::string::npos) {
            request.method_name = msg.substr(pos, end - pos);
        }
    }
    
    // 解析 serializer_type
    pos = msg.find("\"serializer_type\":\"");
    if (pos != std::string::npos) {
        pos += 19; // 跳过 "serializer_type":"
        size_t end = msg.find("\"", pos);
        if (end != std::string::npos) {
            request.serializer_type = msg.substr(pos, end - pos);
        }
    }
    
    // 解析 request_data
    pos = msg.find("\"request_data\":\"");
    if (pos != std::string::npos) {
        pos += 16; // 跳过 "request_data":"
        
        // 找到正确的结束位置，需要处理转义字符
        size_t end = pos;
        while (end < msg.length()) {
            if (msg[end] == '\\') {
                end += 2; // 跳过转义字符
            } else if (msg[end] == '"') {
                break; // 找到未转义的引号
            } else {
                end++;
            }
        }
        
        if (end < msg.length()) {
            std::string escaped_data = msg.substr(pos, end - pos);
            // 反转义JSON字符串
            std::string unescaped_data = escaped_data;
            size_t upos = 0;
            while ((upos = unescaped_data.find("\\\"", upos)) != std::string::npos) {
                unescaped_data.replace(upos, 2, "\"");
                upos += 1;
            }
            upos = 0;
            while ((upos = unescaped_data.find("\\\\", upos)) != std::string::npos) {
                unescaped_data.replace(upos, 2, "\\");
                upos += 1;
            }
            request.request_data = unescaped_data;
        }
    }
    
    // 解析 request_id
    pos = msg.find("\"request_id\":");
    if (pos != std::string::npos) {
        pos += 13; // 跳过 "request_id":
        size_t end = msg.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string id_str = msg.substr(pos, end - pos);
            request.request_id = std::stoll(id_str);
        }
    }
    
    return request;
}

std::string RpcServer::SerializeResponse(const RpcResponseData& response, const std::string& serializer_type) {
    if (serializer_type == "protobuf") {
        // 对于protobuf，我们需要创建一个protobuf消息来序列化整个响应
        // 这里简化处理，仍然使用JSON格式包装，但标记为protobuf
        // 在实际项目中，可以定义专门的protobuf响应消息
        std::cout << "Server: Using protobuf response format (simplified as JSON wrapper)" << std::endl;
    }
    
    // 转义response_data中的特殊字符（只有在JSON格式时需要）
    std::string escaped_response_data = response.response_data;
    if (serializer_type == "json" || serializer_type == "auto") {
        size_t pos = 0;
        while ((pos = escaped_response_data.find("\"", pos)) != std::string::npos) {
            escaped_response_data.replace(pos, 1, "\\\"");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped_response_data.find("\\", pos)) != std::string::npos) {
            if (pos + 1 < escaped_response_data.length() && escaped_response_data[pos + 1] != '"') {
                escaped_response_data.replace(pos, 1, "\\\\");
                pos += 2;
            } else {
                pos += 1;
            }
        }
    }
    
    // 转义error_message中的特殊字符
    std::string escaped_error_message = response.error_message;
    size_t pos = 0;
    while ((pos = escaped_error_message.find("\"", pos)) != std::string::npos) {
        escaped_error_message.replace(pos, 1, "\\\"");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_error_message.find("\\", pos)) != std::string::npos) {
        if (pos + 1 < escaped_error_message.length() && escaped_error_message[pos + 1] != '"') {
            escaped_error_message.replace(pos, 1, "\\\\");
            pos += 2;
        } else {
            pos += 1;
        }
    }
    
    // 构建响应JSON（包含序列化类型信息）
    std::ostringstream oss;
    oss << "{"
        << "\"request_id\":" << response.request_id << ","
        << "\"status_code\":" << static_cast<int>(response.status_code) << ","
        << "\"error_message\":\"" << escaped_error_message << "\","
        << "\"response_data\":\"" << escaped_response_data << "\","
        << "\"serializer_type\":\"" << serializer_type << "\""
        << "}";
    return oss.str();
}

// RpcClient 实现
RpcClient::RpcClient(EventLoop* loop) 
    : loop_(loop), request_id_counter_(0) {
    client_ = std::make_unique<TcpClient>(loop);
    client_->SetConnectionCallback([this](std::shared_ptr<Connection> conn) {
        HandleConnection(conn);
    });
    client_->SetMessageCallback([this](std::shared_ptr<Connection> conn, const std::string& message) {
        HandleMessage(conn, message);
    });
}

RpcClient::~RpcClient() {
    Disconnect();
}

void RpcClient::Connect(const std::string& ip, int port) {
    client_->Connect(ip, port);
}

void RpcClient::Disconnect() {
    client_->Disconnect();
}

void RpcClient::HandleConnection(std::shared_ptr<Connection> conn) {
    std::cout << "Connected to RPC server, fd: " << conn->GetFd() << std::endl;
}

void RpcClient::HandleMessage(std::shared_ptr<Connection> /* conn */, const std::string& message) { // 根据表格找到对应的promise，设置值 
    try {
        // 解码消息
        std::string buffer = message;
        auto messages = MessageCodec::Decode(buffer);
        
        for (const auto& msg : messages) {
            std::cout << "Client: Received response message: " << msg << std::endl;
            RpcResponseData response = ParseResponse(msg);
            std::cout << "Client: Parsed response - RequestID: " << response.request_id 
                      << ", Status: " << static_cast<int>(response.status_code)
                      << ", ResponseData: " << response.response_data << std::endl;
            
            // 查找对应的promise
            std::lock_guard<std::mutex> lock(pending_requests_mutex_);
            auto it = pending_requests_.find(response.request_id);
            if (it != pending_requests_.end()) {
                it->second.set_value(response);
                pending_requests_.erase(it);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling RPC response: " << e.what() << std::endl;
    }
}

int64_t RpcClient::GenerateRequestId() {
    return ++request_id_counter_;
}

std::future<RpcResponseData> RpcClient::SendRequest(const RpcRequestData& request) {
    std::promise<RpcResponseData> promise;
    std::future<RpcResponseData> future = promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(pending_requests_mutex_);
        pending_requests_[request.request_id] = std::move(promise);
    }
    
    std::string request_message = SerializeRequest(request);
    std::string encoded_request = MessageCodec::Encode(request_message);
    client_->Send(encoded_request);
    
    return future;
}

std::string RpcClient::SerializeRequest(const RpcRequestData& request) {
    // 转义JSON字符串中的特殊字符
    std::string escaped_data = request.request_data;
    size_t pos = 0;
    while ((pos = escaped_data.find("\"", pos)) != std::string::npos) {
        escaped_data.replace(pos, 1, "\\\"");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped_data.find("\\", pos)) != std::string::npos) {
        if (pos + 1 < escaped_data.length() && escaped_data[pos + 1] != '"') {
            escaped_data.replace(pos, 1, "\\\\");
            pos += 2;
        } else {
            pos += 1;
        }
    }
    
    std::ostringstream oss;
    oss << "{"
        << "\"service_name\":\"" << request.service_name << "\","
        << "\"method_name\":\"" << request.method_name << "\","
        << "\"serializer_type\":\"" << request.serializer_type << "\","
        << "\"request_data\":\"" << escaped_data << "\","
        << "\"request_id\":" << request.request_id
        << "}";
    return oss.str();
}

RpcResponseData RpcClient::ParseResponse(const std::string& message) {
    // 简单的JSON解析
    RpcResponseData response;
    
    std::string msg = message;
    
    // 解析 request_id
    size_t pos = msg.find("\"request_id\":");
    if (pos != std::string::npos) {
        pos += 13; // 跳过 "request_id":
        size_t end = msg.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string id_str = msg.substr(pos, end - pos);
            response.request_id = std::stoll(id_str);
        }
    }
    
    // 解析 status_code
    pos = msg.find("\"status_code\":");
    if (pos != std::string::npos) {
        pos += 14; // 跳过 "status_code":
        size_t end = msg.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string status_str = msg.substr(pos, end - pos);
            response.status_code = static_cast<RpcStatus>(std::stoi(status_str));
        }
    }
    
    // 解析 error_message
    pos = msg.find("\"error_message\":\"");
    if (pos != std::string::npos) {
        pos += 17; // 跳过 "error_message":"
        size_t end = msg.find("\"", pos);
        if (end != std::string::npos) {
            response.error_message = msg.substr(pos, end - pos);
        }
    }
    
    // 解析 response_data
    pos = msg.find("\"response_data\":\"");
    if (pos != std::string::npos) {
        pos += 17; // 跳过 "response_data":"
        
        // 找到正确的结束位置，需要处理转义字符
        size_t end = pos;
        int brace_count = 0;
        bool in_string = false;
        bool escaped = false;
        
        // 如果response_data以{开头，说明是JSON对象，需要找到匹配的}
        if (pos < msg.length() && msg[pos] == '{') {
            brace_count = 1;
            end = pos + 1;
            
            while (end < msg.length() && brace_count > 0) {
                char c = msg[end];
                
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_string = !in_string;
                } else if (!in_string) {
                    if (c == '{') {
                        brace_count++;
                    } else if (c == '}') {
                        brace_count--;
                    }
                }
                end++;
            }
        } else {
            // 简单字符串，找到下一个未转义的引号
            while (end < msg.length()) {
                if (msg[end] == '\\') {
                    end += 2; // 跳过转义字符
                } else if (msg[end] == '"') {
                    break; // 找到未转义的引号
                } else {
                    end++;
                }
            }
        }
        
        if (end <= msg.length()) {
            std::string raw_data = msg.substr(pos, end - pos);
            
            // 检查是否有转义字符，如果有则反转义
            if (raw_data.find("\\\"") != std::string::npos || raw_data.find("\\\\") != std::string::npos) {
                std::string unescaped_data = raw_data;
                size_t upos = 0;
                while ((upos = unescaped_data.find("\\\"", upos)) != std::string::npos) {
                    unescaped_data.replace(upos, 2, "\"");
                    upos += 1;
                }
                upos = 0;
                while ((upos = unescaped_data.find("\\\\", upos)) != std::string::npos) {
                    unescaped_data.replace(upos, 2, "\\");
                    upos += 1;
                }
                response.response_data = unescaped_data;
            } else {
                response.response_data = raw_data;
            }
        }
    }
    
    // 解析 serializer_type
    pos = msg.find("\"serializer_type\":\"");
    if (pos != std::string::npos) {
        pos += 19; // 跳过 "serializer_type":"
        size_t end = msg.find("\"", pos);
        if (end != std::string::npos) {
            response.serializer_type = msg.substr(pos, end - pos);
        }
    }
    
    return response;
} 