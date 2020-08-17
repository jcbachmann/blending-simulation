cmake_minimum_required(VERSION 3.5)

add_library(BlendingSimulatorLib INTERFACE)

target_compile_features(BlendingSimulatorLib INTERFACE cxx_std_17)

target_include_directories(
	BlendingSimulatorLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
