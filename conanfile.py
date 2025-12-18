from conan import ConanFile

class MyProjectConan(ConanFile):
    name = "rtsp"
    version = "1.0"
    # 需要依赖的库
    # 本地的依赖库和远程库的依赖关系
    requires = (
   #     "libv4l/1.0@user/channel",  # 本地库
        "zlib/1.2.11",              # 远程库（Conan自动从remote下载）
        "libalsa/1.2.13",  
    )

    settings = "os", "arch", "compiler", "build_type"


    def source(self):
        # 用Conan把3rdparty里的本地库“导入”到本地仓库
        for dep in self.requires:
            if "user/channel" in str(dep):
                self.run(f"conan export-pkg 3rdparty/{dep.name} {dep.ref} -p=package_folder=3rdparty/{dep.name}")

    def build(self):
        pass