cmake_minimum_required(VERSION 3.10)
project(RtspSdkTests CXX)

# 查找 GTest
find_package(GTest REQUIRED)

# 测试源文件
file(GLOB TEST_SOURCES "*.cpp")

# 排除test_main.cpp，因为它只是用来初始化测试环境
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_main.cpp")

add_executable(run_tests test_main.cpp ${TEST_SOURCES})

# 设置包含目录
target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/base
    ${CMAKE_SOURCE_DIR}/src/net
)
# 链接库
target_link_libraries(run_tests
    PRIVATE
        rtsp_sdk_static
        GTest::GTest
        GTest::Main
        pthread
)

# 启用测试
enable_testing()
add_test(NAME AllTests COMMAND run_tests)

# 安装规则
install(TARGETS run_tests 
        RUNTIME DESTINATION bin/tests)