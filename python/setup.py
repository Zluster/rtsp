import os
import sys
import subprocess
import platform
from pathlib import Path
from setuptools import setup, find_packages, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11

class CMakeBuildExt(build_ext):
    def build_extensions(self):
        # 确保先构建 C++ 库
        self.build_cpp_library()
        # 然后构建扩展
        super().build_extensions()

    def build_cpp_library(self):
        # 获取源码根目录
        source_dir = Path(__file__).parent.parent.absolute()
        build_dir = Path(self.build_temp) / "cpp_build"
        
        # 创建构建目录
        build_dir.mkdir(parents=True, exist_ok=True)
        
        # 运行 CMake 配置
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={source_dir}/artifacts/libraries",
            f"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY={source_dir}/artifacts/libraries",
            f"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY={source_dir}/artifacts/binaries",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]
        
        # 运行 Conan 安装
        subprocess.check_call([
            "conan", "install", str(source_dir),
            "--build=missing"
        ], cwd=build_dir)
        
        # 配置项目
        subprocess.check_call([
            "cmake", str(source_dir)
        ] + cmake_args, cwd=build_dir)
        
        # 构建项目
        subprocess.check_call([
            "cmake", "--build", ".", "--config", "Release"
        ], cwd=build_dir)

ext_modules = [
    Pybind11Extension(
        "rtsp._rtsp",
        ["../src/python/py_rtsp.cpp"],
        include_dirs=[
            "../src",
            "../src/base", 
            "../src/net",
        ],
        libraries=["rtsp_sdk_static"],
        library_dirs=["../artifacts/libraries"],
        define_macros=[("VERSION_INFO", "10.0.0")],
    ),
]

# 读取 README
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text(encoding='utf-8')

setup(
    name="rtsp-sdk",
    version="0.0.0.1",
    author="zwz",
    author_email="zwz@example.com",
    description="RTSP SDK Python bindings",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/Zluster/rtsp",
    packages=find_packages(),
    ext_modules=ext_modules,
    cmdclass={"build_ext": CMakeBuildExt},
    zip_safe=False,
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: C++",
    ],
    python_requires=">=3.7",
    install_requires=[
        "pybind11>=2.10.0"
    ],
    setup_requires=[
        "pybind11>=2.10.0"
    ]
)