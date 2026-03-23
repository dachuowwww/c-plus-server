find_package(Protobuf REQUIRED)
find_package(mysql-concpp QUIET)

if(NOT TARGET mysql::concpp)
    find_path(MYSQLCPPCONN_INCLUDE_DIR
        NAMES mysql_driver.h
        PATH_SUFFIXES mysql-cppconn jdbc
    )
    find_library(MYSQLCPPCONN_LIBRARY NAMES mysqlcppconn)

    if(NOT MYSQLCPPCONN_INCLUDE_DIR OR NOT MYSQLCPPCONN_LIBRARY)
        message(FATAL_ERROR
            "MySQL Connector/C++ not found. Install mysql-concpp (preferred) or libmysqlcppconn-dev, "
            "or set mysql-concpp_DIR / CMAKE_PREFIX_PATH."
        )
    endif()

    add_library(mysql::concpp UNKNOWN IMPORTED) # 外部库
    set_target_properties(mysql::concpp PROPERTIES
        IMPORTED_LOCATION "${MYSQLCPPCONN_LIBRARY}" # 库文件路径
        INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCPPCONN_INCLUDE_DIR}" # 头文件目录,PINE_SHARED获得头文件搜索路径
    )
endif()

if(NOT TARGET hiredis::hiredis)
    find_path(HIREDIS_INCLUDE_DIR
        NAMES hiredis/hiredis.h
    )
    find_library(HIREDIS_LIBRARY NAMES hiredis)

    if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
        message(FATAL_ERROR
            "hiredis development files not found. Install libhiredis-dev, "
            "or set HIREDIS_INCLUDE_DIR / HIREDIS_LIBRARY."
        )
    endif()

    add_library(hiredis::hiredis UNKNOWN IMPORTED)
    set_target_properties(hiredis::hiredis PROPERTIES
        IMPORTED_LOCATION "${HIREDIS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HIREDIS_INCLUDE_DIR}"
    )
endif()

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
target_link_libraries(PINE_SHARED
PUBLIC
${Protobuf_LIBRARIES}
PRIVATE
mysql::concpp
hiredis::hiredis
)
