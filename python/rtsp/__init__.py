"""
RTSP SDK Python Bindings
"""

# 延迟导入 C++ 扩展模块
def __getattr__(name):
    """延迟加载 C++ 扩展模块中的属性"""
    if name == "_rtsp":
        try:
            from . import _rtsp
            return _rtsp
        except ImportError as e:
            raise ImportError(f"RTSP SDK C++ extension not available: {e}")
    
    # 尝试从 C++ 模块获取属性
    try:
        from . import _rtsp
        return getattr(_rtsp, name)
    except ImportError:
        raise AttributeError(f"module '{__name__}' has no attribute '{name}'")

# 从 core 模块导入高级封装
from .core import *

__all__ = [
    # C++ 绑定类（将在运行时从 _rtsp 模块导入）
    "Logger",
    "EventLoop",
    "InetAddress", 
    "TcpServer",
    "TcpClient",
]