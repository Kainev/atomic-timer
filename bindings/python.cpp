// bindings/bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // For automatic conversion of STL containers
#include "AtomicTimer.h"

namespace py = pybind11;

// Optional: If your classes use std::chrono, etc., include necessary pybind11 headers or converters

PYBIND11_MODULE(atomic_timer, m) {
    m.doc() = "Python bindings for the AtomicTimer C++ library";

    // Binding the AtomicTimer class
    py::class_<AtomicTimer>(m, "AtomicTimer")
        .def(py::init<const std::vector<std::string>&, double, double>(),
             py::arg("servers"),
             py::arg("sync_rate"),
             py::arg("slew_rate"))
        .def("start", &AtomicTimer::start, "Start the timer")
        .def("stop", &AtomicTimer::stop, "Stop the timer and get elapsed time")
        .def("real_time", &AtomicTimer::real_time, "Get the real elapsed time")
        .def("smooth_time", &AtomicTimer::smooth_time, "Get the smoothed elapsed time")
        .def("local_time", &AtomicTimer::local_time, "Get the local elapsed time");
    
    // If you have other classes like NTPClient or Timer to expose, bind them similarly
    // Example:
    // py::class_<NTPClient>(m, "NTPClient")
    //     .def(py::init<std::string, unsigned short>(),
    //          py::arg("hostname"),
    //          py::arg("port") = 123)
    //     .def("connect", &NTPClient::connect, "Connect to the NTP server")
    //     .def("disconnect", &NTPClient::disconnect, "Disconnect from the NTP server")
    //     .def("offset", &NTPClient::offset, "Get the time offset");
}
