#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "net/EventLoop.hpp"
#include "net/TcpClient.hpp"
#include "net/TcpServer.hpp"
#include "base/Logger.hpp"

namespace py = pybind11;
using namespace net;
using namespace base;

PYBIND11_MODULE(rtsp, m)
{
    m.doc() = "RTSP SDK Python bindings";

    // Logger bindings
    py::class_<Logger>(m, "Logger")
        .def_static("debug", [](const std::string &msg)
                    { LOG_DEBUG("%s", msg.c_str()); })
        .def_static("info", [](const std::string &msg)
                    { LOG_INFO("%s", msg.c_str()); })
        .def_static("warn", [](const std::string &msg)
                    { LOG_WARN("%s", msg.c_str()); })
        .def_static("error", [](const std::string &msg)
                    { LOG_ERROR("%s", msg.c_str()); })
        .def_static("fatal", [](const std::string &msg)
                    { LOG_FATAL("%s", msg.c_str()); });

    // EventLoop bindings
    py::class_<EventLoop>(m, "EventLoop")
        .def(py::init<>())
        .def("loop", &EventLoop::loop)
        .def("quit", &EventLoop::quit)
        .def("isInLoopThread", &EventLoop::isInLoopThread)
        .def("assertInLoopThread", &EventLoop::assertInLoopThread);

    // InetAddress bindings
    py::class_<InetAddress>(m, "InetAddress")
        .def(py::init<uint16_t>(), py::arg("port"))
        .def(py::init<const std::string &, uint16_t>(), py::arg("ip"), py::arg("port"))
        .def("toIpPort", &InetAddress::toIpPort);

    // TcpServer bindings (简化版)
    py::class_<TcpServer>(m, "TcpServer")
        .def(py::init<EventLoop *, const InetAddress &, const std::string &>(),
             py::arg("loop"), py::arg("listenAddr"), py::arg("name"))
        .def("start", &TcpServer::start);

    // TcpClient bindings (简化版)
    py::class_<TcpClient>(m, "TcpClient")
        .def(py::init<EventLoop *, const InetAddress &, const std::string &>(),
             py::arg("loop"), py::arg("serverAddr"), py::arg("name"))
        .def("connect", &TcpClient::connect)
        .def("disconnect", &TcpClient::disconnect);
}