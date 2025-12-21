# 查找 pybind11 - 通过 Conan 生成的配置
find_package(pybind11 CONFIG REQUIRED)

# 收集 Python 绑定源文件
file(GLOB PYTHON_BINDINGS_SOURCES "*.cpp")

# 创建 Python 模块
pybind11_add_module(_rtsp ${PYTHON_BINDINGS_SOURCES})

target_include_directories(_rtsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../base # base/ 目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../net  # net/ 目录
)
# 链接主库
target_link_libraries(_rtsp PRIVATE rtsp_sdk_static)

# 设置模块属性
set_target_properties(_rtsp PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX "${PYTHON_MODULE_EXTENSION}"
)
