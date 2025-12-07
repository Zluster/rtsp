file(GLOB_RECURSE SDK_SOURCES "*.cpp")

# 定义核心库的头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/base
                    ${CMAKE_CURRENT_SOURCE_DIR}/rtsp
                    ${CMAKE_CURRENT_SOURCE_DIR}/rtp
                    ${CMAKE_CURRENT_SOURCE_DIR}/net
                    )

# !!! 关键：创建名为 rtsp_sdk 的库目标 !!!
add_library(rtsp_sdk_obj OBJECT ${SDK_SOURCES})

target_compile_options(rtsp_sdk_obj PRIVATE -Wall -Wextra -fPIC) 

add_library(rtsp_sdk_static STATIC $<TARGET_OBJECTS:rtsp_sdk_obj>)
target_link_libraries(rtsp_sdk_static pthread)


add_library(rtsp_sdk_shared SHARED $<TARGET_OBJECTS:rtsp_sdk_obj>)
target_link_libraries(rtsp_sdk_shared pthread)

add_library(rtsp_sdk ALIAS rtsp_sdk_shared)