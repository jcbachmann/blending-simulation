cmake_minimum_required(VERSION 3.9)

add_library(BlendingSimulatorDetailedLib INTERFACE)

hunter_add_package(bullet)
find_package(bullet CONFIG REQUIRED)

target_include_directories(BlendingSimulatorDetailedLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(BlendingSimulatorDetailedLib INTERFACE cxx_std_14)

target_link_libraries(
	BlendingSimulatorDetailedLib INTERFACE
	BlendingSimulatorLib
	bullet::BulletCollision
	bullet::BulletDynamics
	bullet::BulletSoftBody
	bullet::LinearMath
)
