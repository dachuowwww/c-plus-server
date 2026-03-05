cmake_minimum_required(VERSION 3.10)
project(SerializationFramework)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译类型和调试标志
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# 查找依赖包
find_package(Protobuf)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

# 如果pkg-config没找到，尝试手动查找jsoncpp
if(NOT JSONCPP_FOUND)
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES jsoncpp)
    find_library(JSONCPP_LIBRARY jsoncpp
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    
    if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
        set(JSONCPP_FOUND TRUE)
        set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
        set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
    endif()
endif()

# 创建序列化框架库
add_library(serialization_framework
    # serialization_framework.cpp
    # serialization_framework.h
    json_serializer.h
    type_registry.h
    user_serializers.cpp
    user_serializers.h
    user_info.h
)

# 创建网络库
add_library(network
    network.cpp
    network.h
)

# 创建RPC框架库
add_library(rpc_framework
    rpc_framework.cpp
    rpc_framework.h
)
include_directories(/usr/include/jsoncpp)
# 设置库之间的依赖关系
target_link_libraries(rpc_framework serialization_framework network)

# 如果找到了protobuf，添加protobuf序列化器
if(Protobuf_FOUND)
    # 编译protobuf文件
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS user.proto)
    
    # 添加生成的protobuf文件到库
    target_sources(serialization_framework PRIVATE 
        # protobuf_serializer.h 
        ${PROTO_SRCS} 
        ${PROTO_HDRS}
    )
    target_link_libraries(serialization_framework ${Protobuf_LIBRARIES})
    target_include_directories(serialization_framework PRIVATE 
        ${Protobuf_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}  # 包含生成的protobuf头文件
    )
endif()

# 如果找到了jsoncpp，添加jsoncpp支持
if(JSONCPP_FOUND)
    target_link_libraries(serialization_framework ${JSONCPP_LIBRARIES})
    target_include_directories(serialization_framework PRIVATE ${JSONCPP_INCLUDE_DIRS})
    target_compile_definitions(serialization_framework PRIVATE HAVE_JSONCPP)
else()
    # 如果没有找到jsoncpp，尝试直接链接
    find_library(JSONCPP_LIB jsoncpp)
    if(JSONCPP_LIB)
        target_link_libraries(serialization_framework ${JSONCPP_LIB})
        target_compile_definitions(serialization_framework PRIVATE HAVE_JSONCPP)
    endif()
endif()

 

 

add_executable(rpc main.cpp user_type_serializers.cpp)
target_link_libraries(rpc rpc_framework serialization_framework network)
 
target_link_libraries(rpc ${Protobuf_LIBRARIES})
target_include_directories(rpc PRIVATE 
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}  # 包含生成的protobuf头文件
)


 
 
 
# 编译选项
target_compile_options(serialization_framework PRIVATE -Wall -Wextra)
target_compile_options(network PRIVATE -Wall -Wextra)
target_compile_options(rpc_framework PRIVATE -Wall -Wextra)
 
# 包含目录
target_include_directories(serialization_framework PUBLIC .)
target_include_directories(network PUBLIC .)
target_include_directories(rpc_framework PUBLIC .)
 

# 安装规则
install(TARGETS serialization_framework DESTINATION lib)
install(FILES 
    serialization_framework.h 
    json_serializer.h 
    protobuf_serializer.h 
    DESTINATION include)

# 打印配置信息
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(Protobuf_FOUND)
    message(STATUS "Protobuf found: ${Protobuf_VERSION}")
else()
    message(STATUS "Protobuf not found - protobuf serializer will be disabled")
endif()

if(JSONCPP_FOUND)
    message(STATUS "JsonCpp found: ${JSONCPP_VERSION}")
elseif(JSONCPP_LIB)
    message(STATUS "JsonCpp library found: ${JSONCPP_LIB}")
else()
    message(STATUS "JsonCpp not found - JSON serializer will use simple implementation")
endif() 