cmake_minimum_required(VERSION 3.9)

project(BlendingSimulatorLibPython VERSION 1.0.0 DESCRIPTION "Blending Simulator Lib for Python")

hunter_add_package(pybind11)
find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(blending_simulator_lib BlendingSimulatorLibPython.cpp)

target_link_libraries(
	blending_simulator_lib PRIVATE
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	pthread
	pybind11::pybind11
	pybind11::embed
	pybind11::module
)
