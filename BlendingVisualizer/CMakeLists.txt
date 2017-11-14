cmake_minimum_required(VERSION 3.7)

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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/lib/OGRE/cmake/)

if (CMAKE_BUILD_TYPE STREQUAL "")
	# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
	# differentiation between debug and release builds.
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")

find_package(OGRE REQUIRED)
find_package(SDL2 REQUIRED)

# Statically linking boost to a dynamic Ogre build doesn't work on Linux 64bit
set(Boost_USE_STATIC_LIBS ${OGRE_STATIC})
set(Boost_ADDITIONAL_VERSIONS "1.44" "1.44.0" "1.42" "1.42.0" "1.41.0" "1.41" "1.40.0" "1.40" "1.39.0" "1.39" "1.38.0" "1.38" "1.37.0" "1.37")
# Components that need linking (NB does not include header-only components like bind)
set(OGRE_BOOST_COMPONENTS thread date_time)
find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
if (NOT Boost_FOUND)
	# Try again with the other type of libs
	set(Boost_USE_STATIC_LIBS NOT ${Boost_USE_STATIC_LIBS})
	find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
endif ()
find_package(Boost QUIET)

# Set up referencing of Boost
include_directories(${Boost_INCLUDE_DIR})
add_definitions(-DBOOST_ALL_NO_LIB)

include_directories(
	${SDL2_INCLUDE_DIRS}
	${OGRE_INCLUDE_DIRS}
	${OGRE_Overlay_INCLUDE_DIRS}
	${OGRE_Bites_INCLUDE_DIRS}
)

add_library(Visualizer STATIC ${SOURCE_FILES})

set_target_properties(Visualizer PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries(
	Visualizer
	BlendingSimulatorLib
	${Boost_LIBRARIES}
	${SDL2_LIBRARIES}
	${OGRE_LIBRARIES}
	${OGRE_Overlay_LIBRARIES}
	${OGRE_Terrain_LIBRARIES}
	${OGRE_Bites_LIBRARIES}
)

target_include_directories(
	Visualizer PUBLIC
	${SDL2_INCLUDE_DIRS}
	${OGRE_INCLUDE_DIRS}
	${OGRE_Overlay_INCLUDE_DIRS}
	${OGRE_Bites_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

# TODO include media in project
#install(
#	DIRECTORY ${CMAKE_SOURCE_DIR}/dist/media
#	DESTINATION ./
#	CONFIGURATIONS Release RelWithDebInfo Debug
#)
#
#install(
#	FILES ${CMAKE_SOURCE_DIR}/dist/bin/plugins.cfg
#	${CMAKE_SOURCE_DIR}/dist/bin/resources.cfg
#	DESTINATION ${PROJECT_BINARY_DIR}
#	CONFIGURATIONS Release RelWithDebInfo Debug
#)
