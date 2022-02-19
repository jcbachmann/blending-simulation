cmake_minimum_required(VERSION 3.5)

add_library(BlendingSimulatorFastLib INTERFACE)

target_include_directories(
	BlendingSimulatorFastLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(BlendingSimulatorFastLib INTERFACE cxx_std_17)

target_link_libraries(
	BlendingSimulatorFastLib
	INTERFACE BlendingSimulatorLib
)

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()
