find_package(OGRE QUIET)

if (NOT OGRE_FOUND)
	if (WIN32)
		set(CMAKE_MODULE_PATH "$ENV{OGRE_HOME}/CMake/;${CMAKE_MODULE_PATH}")
	elseif (UNIX)
		if (EXISTS "/usr/local/lib/OGRE/cmake")
			set(CMAKE_MODULE_PATH "/usr/local/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
		elseif (EXISTS "/usr/lib/OGRE/cmake")
			set(CMAKE_MODULE_PATH "/usr/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
		elseif (EXISTS "/usr/share/OGRE/cmake/modules")
			set(CMAKE_MODULE_PATH "/usr/share/OGRE/cmake/modules/;${CMAKE_MODULE_PATH}")
		else ()
			message(FATAL_ERROR "Failed to find OGRE cmake module path")
		endif ()
	else ()
		message(FATAL_ERROR "Failed to detect system")
	endif ()
endif ()
