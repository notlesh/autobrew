cmake_minimum_required(VERSION 2.8)

project(pid_temp_controller)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
include_directories(
	${Boost_INCLUDE_DIR}
	"../../core/include"
	"../../../roller/include"
	"../../../devman/include"
	"../../../owfs_devman/include"
	"../../../raspi_gpio_devman/include"
)

AUX_SOURCE_DIRECTORY("src/" src_files)

add_executable(pid_temp_controller ${src_files})

target_link_libraries(pid_temp_controller ${Boost_LIBRARIES})
