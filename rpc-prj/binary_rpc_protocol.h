#pragma once
#ifdef OS_LINUX
#include <arpa/inet.h>
#endif
#ifdef OS_MACOS
#include <winsock2.h>
#endif
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
// 后续可以考虑把RPC的协议从json改成二进制RPC协议
// 二进制RPC协议设计
// 
// 协议格式：
// +--------+--------+--------+--------+--------+--------+--------+--------+
// | Magic  | Version| MsgType| Flags  |           Message Length          |
// +--------+--------+--------+--------+--------+--------+--------+--------+
// |                          Request ID                                   |
// +--------+--------+--------+--------+--------+--------+--------+--------+
// |SerType | ServiceName Len |          Service Name                      |
// +--------+--------+--------+--------+--------+--------+--------+--------+
// |MethodName Len   |              Method Name                           |
// +--------+--------+--------+--------+--------+--------+--------+--------+
// |                        Request Data Length                           |
// +--------+--------+--------+--------+--------+--------+--------+--------+
// |                          Request Data                                |
// +--------+--------+--------+--------+--------+--------+--------+--------+

namespace BinaryRpc {

// 协议常量
static const uint16_t MAGIC_NUMBER = 0x5250;  // "RP" in hex
static const uint8_t PROTOCOL_VERSION = 1;

// 消息类型
enum class MessageType : uint8_t {
    REQUEST = 1,
    RESPONSE = 2,
    ERROR = 3
};

// 序列化类型
enum class SerializerType : uint8_t {
    JSON = 1,
    PROTOBUF = 2,
    MESSAGEPACK = 3
};

// RPC状态码
enum class StatusCode : uint32_t {
    SUCCESS = 0,
    METHOD_NOT_FOUND = 1,
    SERIALIZATION_ERROR = 2,
    NETWORK_ERROR = 3,
    TIMEOUT = 4,
    INVALID_REQUEST = 5,
    UNKNOWN_ERROR = 6
};

// 协议头结构
struct ProtocolHeader {
    uint16_t magic;           // 魔数
    uint8_t version;          // 协议版本
    uint8_t message_type;     // 消息类型
    uint8_t flags;            // 标志位（保留）
    uint8_t serializer_type;  // 序列化类型
    uint16_t reserved;        // 保留字段
    uint32_t message_length;  // 消息总长度
    uint64_t request_id;      // 请求ID
} __attribute__((packed));

// 请求消息结构
struct RequestMessage {
    ProtocolHeader header;
    std::string service_name;
    std::string method_name;
    std::vector<uint8_t> request_data;
    
    RequestMessage() {
        header.magic = MAGIC_NUMBER;
        header.version = PROTOCOL_VERSION;
        header.message_type = static_cast<uint8_t>(MessageType::REQUEST);
        header.flags = 0;
        header.reserved = 0;
    }
};

// 响应消息结构
struct ResponseMessage {
    ProtocolHeader header;
    uint32_t status_code;
    std::string error_message;
    std::vector<uint8_t> response_data;
    
    ResponseMessage() {
        header.magic = MAGIC_NUMBER;
        header.version = PROTOCOL_VERSION;
        header.message_type = static_cast<uint8_t>(MessageType::RESPONSE);
        header.flags = 0;
        header.reserved = 0;
    }
};

// 二进制协议编解码器
class BinaryCodec {
public:
    // 编码请求消息
    static std::vector<uint8_t> EncodeRequest(const RequestMessage& request) {
        std::vector<uint8_t> buffer;
        
        // 计算消息长度
        size_t total_length = sizeof(ProtocolHeader) + 
                             1 + request.service_name.length() +  // service_name_len + service_name
                             1 + request.method_name.length() +   // method_name_len + method_name
                             4 + request.request_data.size();     // data_len + data
        
        buffer.reserve(total_length);
        
        // 写入协议头
        ProtocolHeader header = request.header;
        header.message_length = htonl(static_cast<uint32_t>(total_length));
        header.request_id = htobe64(header.request_id);
        
        buffer.insert(buffer.end(), 
                     reinterpret_cast<const uint8_t*>(&header),
                     reinterpret_cast<const uint8_t*>(&header) + sizeof(header));
        
        // 写入service name
        uint8_t service_name_len = static_cast<uint8_t>(request.service_name.length());
        buffer.push_back(service_name_len);
        buffer.insert(buffer.end(), request.service_name.begin(), request.service_name.end());
        
        // 写入method name
        uint8_t method_name_len = static_cast<uint8_t>(request.method_name.length());
        buffer.push_back(method_name_len);
        buffer.insert(buffer.end(), request.method_name.begin(), request.method_name.end());
        
        // 写入request data
        uint32_t data_len = htonl(static_cast<uint32_t>(request.request_data.size()));
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&data_len),
                     reinterpret_cast<const uint8_t*>(&data_len) + sizeof(data_len));
        buffer.insert(buffer.end(), request.request_data.begin(), request.request_data.end());
        
        return buffer;
    }
    
    // 编码响应消息
    static std::vector<uint8_t> EncodeResponse(const ResponseMessage& response) {
        std::vector<uint8_t> buffer;
        
        // 计算消息长度
        size_t total_length = sizeof(ProtocolHeader) +
                             4 +                                    // status_code
                             1 + response.error_message.length() +  // error_msg_len + error_msg
                             4 + response.response_data.size();     // data_len + data
        
        buffer.reserve(total_length);
        
        // 写入协议头
        ProtocolHeader header = response.header;
        header.message_length = htonl(static_cast<uint32_t>(total_length));
        header.request_id = htobe64(header.request_id);
        
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&header),
                     reinterpret_cast<const uint8_t*>(&header) + sizeof(header));
        
        // 写入status code
        uint32_t status = htonl(response.status_code);
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&status),
                     reinterpret_cast<const uint8_t*>(&status) + sizeof(status));
        
        // 写入error message
        uint8_t error_msg_len = static_cast<uint8_t>(response.error_message.length());
        buffer.push_back(error_msg_len);
        buffer.insert(buffer.end(), response.error_message.begin(), response.error_message.end());
        
        // 写入response data
        uint32_t data_len = htonl(static_cast<uint32_t>(response.response_data.size()));
        buffer.insert(buffer.end(),
                     reinterpret_cast<const uint8_t*>(&data_len),
                     reinterpret_cast<const uint8_t*>(&data_len) + sizeof(data_len));
        buffer.insert(buffer.end(), response.response_data.begin(), response.response_data.end());
        
        return buffer;
    }
    
    // 解码请求消息
    static bool DecodeRequest(const std::vector<uint8_t>& buffer, RequestMessage& request) {
        if (buffer.size() < sizeof(ProtocolHeader)) {
            return false;
        }
        
        size_t offset = 0;
        
        // 读取协议头
        memcpy(&request.header, buffer.data(), sizeof(ProtocolHeader));
        request.header.message_length = ntohl(request.header.message_length);
        request.header.request_id = be64toh(request.header.request_id);
        offset += sizeof(ProtocolHeader);
        
        // 验证魔数和版本
        if (request.header.magic != MAGIC_NUMBER || request.header.version != PROTOCOL_VERSION) {
            return false;
        }
        
        // 读取service name
        if (offset >= buffer.size()) return false;
        uint8_t service_name_len = buffer[offset++];
        if (offset + service_name_len > buffer.size()) return false;
        request.service_name.assign(buffer.begin() + offset, buffer.begin() + offset + service_name_len);
        offset += service_name_len;
        
        // 读取method name
        if (offset >= buffer.size()) return false;
        uint8_t method_name_len = buffer[offset++];
        if (offset + method_name_len > buffer.size()) return false;
        request.method_name.assign(buffer.begin() + offset, buffer.begin() + offset + method_name_len);
        offset += method_name_len;
        
        // 读取request data
        if (offset + 4 > buffer.size()) return false;
        uint32_t data_len;
        memcpy(&data_len, buffer.data() + offset, sizeof(data_len));
        data_len = ntohl(data_len);
        offset += 4;
        
        if (offset + data_len > buffer.size()) return false;
        request.request_data.assign(buffer.begin() + offset, buffer.begin() + offset + data_len);
        
        return true;
    }
    
    // 解码响应消息
    static bool DecodeResponse(const std::vector<uint8_t>& buffer, ResponseMessage& response) {
        if (buffer.size() < sizeof(ProtocolHeader)) {
            return false;
        }
        
        size_t offset = 0;
        
        // 读取协议头
        memcpy(&response.header, buffer.data(), sizeof(ProtocolHeader));
        response.header.message_length = ntohl(response.header.message_length);
        response.header.request_id = be64toh(response.header.request_id);
        offset += sizeof(ProtocolHeader);
        
        // 验证魔数和版本
        if (response.header.magic != MAGIC_NUMBER || response.header.version != PROTOCOL_VERSION) {
            return false;
        }
        
        // 读取status code
        if (offset + 4 > buffer.size()) return false;
        memcpy(&response.status_code, buffer.data() + offset, sizeof(response.status_code));
        response.status_code = ntohl(response.status_code);
        offset += 4;
        
        // 读取error message
        if (offset >= buffer.size()) return false;
        uint8_t error_msg_len = buffer[offset++];
        if (offset + error_msg_len > buffer.size()) return false;
        response.error_message.assign(buffer.begin() + offset, buffer.begin() + offset + error_msg_len);
        offset += error_msg_len;
        
        // 读取response data
        if (offset + 4 > buffer.size()) return false;
        uint32_t data_len;
        memcpy(&data_len, buffer.data() + offset, sizeof(data_len));
        data_len = ntohl(data_len);
        offset += 4;
        
        if (offset + data_len > buffer.size()) return false;
        response.response_data.assign(buffer.begin() + offset, buffer.begin() + offset + data_len);
        
        return true;
    }
};

} // namespace BinaryRpc 