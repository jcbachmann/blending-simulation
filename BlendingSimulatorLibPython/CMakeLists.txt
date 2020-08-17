cmake_minimum_required(VERSION 3.9)

project(BlendingSimulatorLibPython VERSION 1.0.0 DESCRIPTION "Blending Simulator Lib for Python")

hunter_add_package(pybind11)
find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(
	blending_simulator_lib
	src/BlendingSimulatorLibPython.cpp
)

set_target_properties(
	blending_simulator_lib PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

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
