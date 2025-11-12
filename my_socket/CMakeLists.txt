# CMake minimum version and project settings
cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(PINE 
VERSION 0.1.0 
DESCRIPTION "Pine is a lightweight C++ project for building high-performance network applications."
LANGUAGES C CXX)

# stardard output
file(TO_CMAKE_PATH "$PROJECT_BINARY_DIR/CMakeLists.txt" PATH_TO_CMAKELISTS_TXT)
if(EXISTS ${PATH_TO_CMAKELISTS_TXT})
    message(FATAL_ERROR "CMakeLists.txt already exists in the build directory. Please use a separate build directory.")    
endif()

# Compiler and linker flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -std=c++20 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-attributes") #TODO: remove
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
set(CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS} -fPIC")
set(GCC_COVERAGE_LINKER_FLAGS "-fPIC")
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(STATUS "Platform is MacOS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_MACOS")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "Platform is Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_LINUX")
endif()
message(STATUS "CMake CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMake CXX Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMake EXE LINKER Flags:${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMake SHARED LINKER Flags:${CMAKE_SHARED_LINKER_FLAGS}")

# Include directories
set(PINE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/include" CACHE PATH "Path to pine source include directory")
# set(PINE_TESTS_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/test/include" CACHE PATH "Path to pine test include directory")
# include_directories(${PINE_SRC_INCLUDE_DIR} ${PINE_TESTS_INCLUDE_DIR})
# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(test)

# Configuration options
set(PINE_BUILD_SUPPORT_DIR "${PROJECT_SOURCE_DIR}/build_support" CACHE PATH "Path to pine build support directory")
set(PINE_CLANG_SEARCH_PATH "/usr/bin" CACHE PATH "Path to clang-10.0.0 bin directory")

# Find external tools
if (NOT DEFINED CLANG_FORMAT_BIN)
    #find_program need variable name
    find_program(CLANG_FORMAT_BIN NAMES clang-format-10 clang-format HINTS ${PINE_CLANG_SEARCH_PATH})
    if(${CLANG_FORMAT_BIN} STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
        message(WARNING "Pine/main couldn't find clang_format.")
        set(CLANG_FORMAT_BIN "")
    else()
        message(STATUS "Pine/main found clang_format at ${CLANG_FORMAT_BIN}")
    endif()
endif()

if(NOT DEFINED CPPLINT_BIN)
    find_program(CPPLINT_BIN NAMES cpplint.py cpplint HINTS ${PINE_BUILD_SUPPORT_DIR})
    if(${CPPLINT_BIN} STREQUAL "CPPLINT_BIN-NOTFOUND")
        message(WARNING "Pine/main couldn't find cpplint.")
        set(CPPLINT_BIN "")
    else()
        message(STATUS "Pine/main found cpplint at ${PINE_BUILD_SUPPORT_DIR}")
    endif()
endif(NOT DEFINED CPPLINT_BIN)

if (NOT DEFINED CLANG_TIDY_BIN)
    find_program(CLANG_TIDY_BIN NAMES clang-tidy-10 clang-tidy HINTS ${PINE_CLANG_SEARCH_PATH})
    if(${CLANG_TIDY_BIN} STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
        message(WARNING "Pine/main couldn't find clang_tidy.")
        set(CLANG_TIDY_BIN "")
    else()
        set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
        message(STATUS "Pine/main found clang_tidy at ${CLANG_TIDY_BIN}")
    endif()
endif()

# Add custom targets for code formatting and linting
#accept directory structure
string(CONCAT PINE_FORMAT_DIRS 
"${CMAKE_SOURCE_DIR}/src,"
"${CMAKE_SOURCE_DIR}/test")

add_custom_target(format ${PINE_BUILD_SUPPORT_DIR}/run_clang_format.py
    ${CLANG_FORMAT_BIN}
    ${PINE_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
    --source_dirs
    ${PINE_FORMAT_DIRS}
    --fix
    --quiet

    COMMENT "Running clang-format on source files..."
)
add_custom_target(check-format ${PINE_BUILD_SUPPORT_DIR}/run_clang_format.py
    ${CLANG_FORMAT_BIN}
    ${PINE_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
    --source_dirs
    ${PINE_FORMAT_DIRS}
    --quiet

    COMMENT "Checking clang-format on source files..."
)
#only accept file structure
file(GLOB_RECURSE PINE_LINT_FILES 
${CMAKE_CURRENT_SOURCE_DIR}/test/*.h
${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

#cancel  <condition_variable> is an unapproved C++11 header.
add_custom_target(cpplint echo '${PINE_LINT_FILES}' | xargs -n12 -P8
        ${CPPLINT_BIN}
        --verbose=2 --quiet
        --linelength=120
        --filter=-legal/copyright,-build/include_subdir,-build/c++11,-readability/casting
        )



add_custom_target(clang-tidy ${PINE_BUILD_SUPPORT_DIR}/run_clang_tidy.py
    -clang-tidy-binary ${CLANG_TIDY_BIN}
    -p ${CMAKE_BINARY_DIR}

    COMMENT "Running clang-tidy on source files..."
)










