cmake_minimum_required(VERSION 3.5)

add_library(BlendingSimulatorFastLib INTERFACE)

target_include_directories(BlendingSimulatorFastLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(BlendingSimulatorFastLib INTERFACE cxx_std_14)

target_link_libraries(
	BlendingSimulatorFastLib
	INTERFACE BlendingSimulatorLib
)
