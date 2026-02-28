if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif ()

if (BUILD_CLI OR BUILD_PYTHON_LIB)
	find_package(Threads REQUIRED)
endif ()

if (BUILD_CLI)
	include(cmake/CLI11.cmake)
endif ()

if (BUILD_TESTS)
	include(cmake/GoogleTest.cmake)
endif ()

if (BUILD_DETAILED_SIMULATOR)
	include(cmake/Bullet.cmake)
endif ()

if (BUILD_VISUALIZER)
	include(cmake/Ogre.cmake)
	include(cmake/SDL2.cmake)
endif ()