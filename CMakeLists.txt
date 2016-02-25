cmake_minimum_required(VERSION 3.3)
project(yaecache)

# compile-time and preprocessor-time definitions
set(PACKAGE "yaecache")
set(VERSION "0.0.1")
set(CMAKE_C_FLAGS "-std=c99 -O2 -pipe -Wall -Wextra")

# configfile template
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

if(NOT CONFIG_H)
	message(FATAL_ERROR "Can't find config.h")
else()
	add_definitions(-DHAVE_CONFIG_H)
endif()

# function checks
include(CheckFunctionExists)
check_function_exists(sigignore HAVE_SIGIGNORE)
check_function_exists(mlockall HAVE_MLOCKALL)

# include file checks
include(CheckIncludeFile)
check_include_file(stdbool.h HAVE_STDBOOL_H)

aux_source_directory(. DIR_SRCS)
add_executable(yaecache ${DIR_SRCS})