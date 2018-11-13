cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	Visualizer.cpp
	Visualizer.h
	QualityColor.cpp
	QualityColor.h
	BlendingVisualizer.impl.h
	BlendingVisualizer.h
	HeapMesh.cpp
	HeapMesh.h
	Noise.cpp
	Noise.h
)

hunter_add_package(SDL2)
find_package(SDL2 CONFIG REQUIRED)

# TODO hunter
include(../cmake/OgreUbuntu.cmake)
find_package(OGRE REQUIRED)

add_library(BlendingVisualizer STATIC ${SOURCE_FILES})

target_link_libraries(
	BlendingVisualizer
	BlendingSimulatorLib
	SDL2::SDL2
	${OGRE_LIBRARIES}
)

target_include_directories(
	BlendingVisualizer
	PRIVATE ${OGRE_INCLUDE_DIRS}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
