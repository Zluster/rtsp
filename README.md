
rtsp/ 
├── build/ # 构建目录（临时文件）
├── cmake/ # CMake相关配置和模块 
├── conan/ # Conan包管理配置 
├── docs/ # 项目文档 
├── examples/ # 示例代码 
├── include/ # 公共头文件 
├── scripts/ # 脚本目录 
├── src/ # 源代码 
├── tests/ # 测试代码 
├── third_party/ # 第三方依赖库 
├── toolchains/ # 工具链配置文件 
├── artifacts/ # 构建产物 
└── README.md # 项目说明文档
编译步骤
1. 生成Conan依赖配置
conan install conan/conanfile.py -s build_type=Release --build=missing
2. 生成cmake工程文件
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../toolchains/linux.toolchain.cmake 
make -j$(nproc)