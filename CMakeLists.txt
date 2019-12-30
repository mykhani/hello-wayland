cmake_minimum_required(VERSION 3.0)

project(hello-wayland)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Wayland 1.10.0 REQUIRED)

add_executable(simple-wl-client main.c)

target_include_directories(
	simple-wl-client
	PRIVATE
	${PROJECT_SOURCE_DIR}
	${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(
	simple-wl-client
	${WAYLAND_LIBRARIES}
)

install(TARGETS simple-wl-client DESTINATION bin)
