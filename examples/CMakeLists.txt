cmake_minimum_required(VERSION 3.10)

# 添加可执行文件
add_executable(tcp_server tcp_server.cpp)
add_executable(tcp_client tcp_client.cpp)

# 设置包含目录
target_include_directories(tcp_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/base
    ${CMAKE_SOURCE_DIR}/src/net)

target_include_directories(tcp_client PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/base
    ${CMAKE_SOURCE_DIR}/src/net)

# 链接库
target_link_libraries(tcp_server PRIVATE rtsp_sdk_static pthread)
target_link_libraries(tcp_client PRIVATE rtsp_sdk_static pthread)

# 安装规则
install(TARGETS tcp_server tcp_client 
        RUNTIME DESTINATION bin)