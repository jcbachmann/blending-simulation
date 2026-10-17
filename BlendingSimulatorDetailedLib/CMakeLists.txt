cmake_minimum_required(VERSION 3.15)

option(USE_OPENMP "Parallelize the detailed simulation with OpenMP where available" ON)

add_library(
	BlendingSimulatorDetailedLib STATIC
	src/ContactModel.cpp
	src/World.cpp
)
add_library(BlendingSimulator::DetailedLib ALIAS BlendingSimulatorDetailedLib)

target_compile_features(BlendingSimulatorDetailedLib PUBLIC cxx_std_17)
set_target_properties(BlendingSimulatorDetailedLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
	BlendingSimulatorDetailedLib
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	BlendingSimulatorDetailedLib
	PUBLIC BlendingSimulator::Lib
)

if (USE_OPENMP)
	find_package(OpenMP)
	if (OpenMP_CXX_FOUND)
		target_link_libraries(BlendingSimulatorDetailedLib PRIVATE OpenMP::OpenMP_CXX)
	endif ()
endif ()

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()

if (BUILD_TOOLS)
	add_subdirectory(tools)
endif ()
