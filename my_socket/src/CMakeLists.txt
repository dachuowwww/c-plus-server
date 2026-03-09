find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROJECT_SOURCE_DIR}/proto/rpc.proto) # 生成可执行文件
set_source_files_properties(${PROTO_SRCS} PROPERTIES SKIP_CLANG_TIDY ON)
file(GLOB_RECURSE PINE_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(PINE_SHARED SHARED ${PINE_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS}) 
#target need variable name
target_include_directories(PINE_SHARED 
PUBLIC
$<BUILD_INTERFACE:${PINE_INCLUDE_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # 引用protobuf文件
$<INSTALL_INTERFACE:include>
PRIVATE
${PINE_SRC_INCLUDE_DIR}
)

target_include_directories(PINE_SHARED PUBLIC ${Protobuf_INCLUDE_DIRS})
target_link_libraries(PINE_SHARED PUBLIC ${Protobuf_LIBRARIES})
