cmake_minimum_required(VERSION 3.0)

project(hello-wayland)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Wayland 1.10.0 REQUIRED)

add_executable(simple-client main.c)

target_include_directories(
	simple-client
	PRIVATE
	${PROJECT_SOURCE_DIR}
	${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(
	simple-client
	${WAYLAND_LIBRARIES}
)
