cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	src/Visualizer.cpp
	src/QualityColor.cpp
	src/HeapMesh.cpp
	src/Noise.cpp
)

add_library(BlendingVisualizer STATIC ${SOURCE_FILES})

set_target_properties(
	BlendingVisualizer PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

hunter_add_package(SDL2)
find_package(SDL2 CONFIG REQUIRED)

# TODO hunter
include(../cmake/OgreUbuntu.cmake)
find_package(OGRE REQUIRED)

target_link_libraries(
	BlendingVisualizer
	BlendingSimulatorLib
	SDL2::SDL2
	${OGRE_LIBRARIES}
)

target_include_directories(
	BlendingVisualizer
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/BlendingSimulator
	PUBLIC ${OGRE_INCLUDE_DIRS}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
