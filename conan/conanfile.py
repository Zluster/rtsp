from conan import ConanFile

class MyProjectConan(ConanFile):
    name = "rtsp"
    version = "0.0.0.1"
    # 需要依赖的库
    # 本地的依赖库和远程库的依赖关系
    requires = (
        "zlib/1.2.11",            
        "libalsa/1.2.13",  
        "pybind11/2.11.1",
        "gtest/1.14.0"
    )

    settings = "os", "arch", "compiler", "build_type"
     
    generators = "CMakeDeps", "CMakeToolchain"

    def source(self):
        # 用Conan把3rdparty里的本地库“导入”到本地仓库
        for dep in self.requires:
            if "user/channel" in str(dep):
                self.run(f"conan export-pkg 3rdparty/{dep.name} {dep.ref} -p=package_folder=3rdparty/{dep.name}")
    def layout(self):
        self.folders.generators = "."

    def generator(self):
        # 使用 CMakeToolchain 和 CMakeDeps
        from conan.tools.cmake import CMakeToolchain, CMakeDeps
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.set_property("cmakedeps_legacy", True)
        deps.generate()
    def build(self):
        pass