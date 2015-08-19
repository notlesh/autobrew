cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

include_directories("include/"
		"../../roller/include"
		"../../devman/include"
)

AUX_SOURCE_DIRECTORY("src/" src_files)

add_library(ab2_core ${src_files})

# target_include_directories(ab2_core PUBLIC &{CMAKE_CURRENT_SOURCE_DIR})
