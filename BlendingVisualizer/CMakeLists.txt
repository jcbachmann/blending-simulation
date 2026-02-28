cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	src/Visualizer.cpp
	src/QualityColor.cpp
	src/HeapMesh.cpp
	src/Noise.cpp
)

add_library(BlendingVisualizer STATIC ${SOURCE_FILES})
add_library(BlendingSimulator::Visualizer ALIAS BlendingVisualizer)

set_target_properties(
	BlendingVisualizer PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingVisualizer
	BlendingSimulator::Lib
	SDL2::SDL2
	Ogre::OgreMain
	Ogre::OgreBites
	Ogre::OgreOverlay
	Ogre::OgreTerrain
)

target_include_directories(
	BlendingVisualizer
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/BlendingSimulator
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
