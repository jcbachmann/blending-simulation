cmake_minimum_required(VERSION 3.9)

add_library(BlendingSimulatorDetailedLib INTERFACE)

target_include_directories(
	BlendingSimulatorDetailedLib INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
	/usr/include/bullet
)

target_link_libraries(
	BlendingSimulatorDetailedLib INTERFACE
	BlendingSimulatorLib
	BulletCollision
	BulletDynamics
	BulletSoftBody
	LinearMath
)
