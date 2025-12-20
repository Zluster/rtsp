# 收集源文件
file(GLOB_RECURSE SDK_SOURCES "*.cpp")

# 创建对象库
add_library(rtsp_sdk_obj OBJECT ${SDK_SOURCES})

# 设置包含目录
target_include_directories(rtsp_sdk_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/net
)

# 编译选项
target_compile_options(rtsp_sdk_obj PRIVATE -Wall -Wextra -fPIC) 

# 创建静态库
add_library(rtsp_sdk_static STATIC $<TARGET_OBJECTS:rtsp_sdk_obj>)
target_link_libraries(rtsp_sdk_static pthread)

# 创建动态库
add_library(rtsp_sdk_shared SHARED $<TARGET_OBJECTS:rtsp_sdk_obj>)
target_link_libraries(rtsp_sdk_shared pthread)

# 创建别名
add_library(rtsp_sdk ALIAS rtsp_sdk_shared)

# 安装规则
install(TARGETS rtsp_sdk_static rtsp_sdk_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# 安装头文件
install(DIRECTORY base/ net/ 
        DESTINATION include/rtsp
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")