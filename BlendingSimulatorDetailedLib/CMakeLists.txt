cmake_minimum_required(VERSION 3.9)

add_library(BlendingSimulatorDetailedLib INTERFACE)
add_library(BlendingSimulator::DetailedLib ALIAS BlendingSimulatorDetailedLib)

target_compile_features(BlendingSimulatorDetailedLib INTERFACE cxx_std_17)

target_include_directories(
	BlendingSimulatorDetailedLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	BlendingSimulatorDetailedLib INTERFACE
	BlendingSimulator::Lib
	Bullet::BulletSoftBody
	Bullet::BulletDynamics
	Bullet::BulletCollision
	Bullet::LinearMath
)

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()
