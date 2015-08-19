cmake_minimum_required(VERSION 2.8)

project(valve_controller)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
include_directories(
	"../../core/include"
	"../../../devman/include"
	"../../../roller/include"
	"../../../owfs_devman/include"
	"../../../raspi_gpio_devman/include"
)

AUX_SOURCE_DIRECTORY("src/" src_files)

message("finding libs...")
message("  PWD (SOURCE): ${CMAKE_CURRENT_SOURCE_DIR}")
message("  PWD (BINARY): ${CMAKE_CURRENT_BINARY_DIR}")
message("  lib suffixes: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
message("  lib prefixes: ${CMAKE_FIND_LIBRARY_PREFIXES}")

find_library(ROLLER_CORE_LIBRARY
	NAMES "roller_core"
	HINTS "../../../roller/core/debug"
	NO_DEFAULT_PATH
)

find_library(DEVMAN_LIBRARY
	NAMES "devman"
	PATHS "../../../devman/build/"
		  "/home/stephen/autobrew/devman/build/"
	NO_DEFAULT_PATH
)

find_library(OWFS_DEVMAN_LIBRARY
	NAMES "owfs_devman"
	HINTS "../../../owfs_devman/build"
	NO_DEFAULT_PATH
)

find_library(RASPI_GPIO_DEVMAN_LIBRARY
	NAMES "raspi_gpio_devman"
	HINTS "../../../raspi_gpio_devman/build"
	NO_DEFAULT_PATH
)

add_executable(valve_controller ${src_files})

# target_link_libraries(valve_controller
# ${ROLLER_CORE_LIBRARY}
# ${DEVMAN_LIBRARY}
# ${OWFS_DEVMAN_LIBRARY}
# ${RASPI_GPIO_DEVMAN_LIBRARY}
# )

target_link_libraries(valve_controller
	# -L../../../devman/build/
	-L/home/stephen/autobrew/devman/build/
	-ldevman
	-L"../../../owfs_devman/build"
	-lowfs_devman
	-L"../../../raspi_gpio_devman/build"
	-lraspi_gpio_devman
)

