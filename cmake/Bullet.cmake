include(FetchContent)

FetchContent_Declare(
	bullet
	URL https://github.com/bulletphysics/bullet3/archive/refs/tags/2.87.tar.gz
)

set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(INSTALL_LIBS ON CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(bullet)

if (TARGET LinearMath)
	target_include_directories(LinearMath PUBLIC $<BUILD_INTERFACE:${bullet_SOURCE_DIR}/src>)
	add_library(Bullet::LinearMath ALIAS LinearMath)
endif ()
if (TARGET BulletCollision)
	target_include_directories(BulletCollision PUBLIC $<BUILD_INTERFACE:${bullet_SOURCE_DIR}/src>)
	add_library(Bullet::BulletCollision ALIAS BulletCollision)
endif ()
if (TARGET BulletDynamics)
	target_include_directories(BulletDynamics PUBLIC $<BUILD_INTERFACE:${bullet_SOURCE_DIR}/src>)
	add_library(Bullet::BulletDynamics ALIAS BulletDynamics)
endif ()
if (TARGET BulletSoftBody)
	target_include_directories(BulletSoftBody PUBLIC $<BUILD_INTERFACE:${bullet_SOURCE_DIR}/src>)
	add_library(Bullet::BulletSoftBody ALIAS BulletSoftBody)
endif ()