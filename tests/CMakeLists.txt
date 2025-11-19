cmake_minimum_required(VERSION 3.10)
project(RtspSdkTests CXX)

# 为了验证，我们可以打印一下 CMAKE_SOURCE_DIR
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

# 查找 gtest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_library(GTEST_LIBRARY NAMES libgtest.a gtest)
find_library(GTEST_MAIN_LIBRARY NAMES libgtest_main.a gtest_main)
# !!! 关键修改：使用 CMAKE_SOURCE_DIR 指向项目根目录 !!!
include_directories(${CMAKE_SOURCE_DIR}/src)

# 测试源文件
file(GLOB TEST_SOURCES "*.cpp")
add_executable(run_tests ${TEST_SOURCES})

# 链接库
target_link_libraries(run_tests
    PRIVATE
        rtsp_sdk_static
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        pthread
)

# 启用测试
enable_testing()
add_test(NAME AllTests COMMAND run_tests)