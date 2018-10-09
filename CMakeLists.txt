cmake_minimum_required(VERSION 3.1)
project(UPCA C CXX)

# determine whether this is a standalone project or included by other projects
set(UPCA_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(UPCA_STANDALONE_PROJECT ON)
endif ()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-c++11-compat")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat-pedantic")
endif()

if(${CMAKE_C_COMPILER_ID} MATCHES "Intel")
  set(COMPILER_IS_INTEL ON)
  execute_process(COMMAND ${CMAKE_C_COMPILER} --version
	OUTPUT_VARIABLE ICC_VERSION_OUTPUT
	ERROR_VARIABLE  ICC_VERSION_OUTPUT
	RESULT_VARIABLE ICC_VERSION_RESULT)
  if(ICC_VERSION_RESULT)
    message(SEND_ERROR "Command \"${CMAKE_C_COMPILER} --version\" failed:\n${ICC_VERSION_RESULT}")
  else()
    STRING(REGEX REPLACE "icc \\(ICC\\) ([0-9]+)\\.[0-9]+.*" "\\1" ICC_MAJOR  "${ICC_VERSION_OUTPUT}")
    if(${ICC_MAJOR} LESS "12")
      message(FATAL_ERROR "At least icc 12.0 is required.")
    endif()
  endif()
endif()
  
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "${CMAKE_SYSTEM_PROCESSOR}")
include(FindPackageHandleStandardArgs)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  find_path(JEVENTS_INCDIR
    NAMES jevents.h
    HINTS ${JEVENTS_DIR}
    PATH_SUFFIXES "include" "include/jevents")
  find_library(JEVENTS_LIBRARIES jevents
    HINTS ${JEVENTS_DIR}
    PATH_SUFFIXES lib lib32 lib64)
  find_package_handle_standard_args(JEVENTS DEFAULT_MSG JEVENTS_LIBRARIES JEVENTS_INCDIR)
  if(JEVENTS_FOUND)
    add_library(jevents INTERFACE)
    target_include_directories(jevents INTERFACE $<BUILD_INTERFACE: ${JEVENTS_INCDIR}>)
    target_link_libraries(jevents INTERFACE $<BUILD_INTERFACE: ${JEVENTS_LIBRARIES}>)
  endif()
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "BlueGene")
  find_path(BGPM_INCDIR
    NAMES bgpm.h
    HINTS ${BGPM_DIR}
    PATH_SUFFIXES bgpm/include)
  find_library(BGPM_LIBRARIES bgpm
    HINTS ${BGPM_DIR}
    PATH_SUFFIXES bgpm/lib lib lib32 lib64)
  find_package_handle_standard_args(BGPM DEFAULT_MSG BGPM_LIBRARIES BGPM_INCDIR)
  if(BGPM_FOUND)
    set(HAVE_BGPM ON)
    add_library(bgpm INTERFACE)
    target_include_directories(bgpm INTERFACE $<BUILD_INTERFACE: ${BGPM_INCDIR}>)
    target_link_libraries(bgpm INTERFACE $<BUILD_INTERFACE: ${BGPM_LIBRARIES}>)
  endif()
endif()

configure_file(config.h.in upca/config.h)
include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(3rd)
add_subdirectory(src)

install(DIRECTORY include/upca DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/upca/config.h DESTINATION include/upca)

if (UPCA_STANDALONE_PROJECT)
  add_executable(example example.cc)
  target_link_libraries(example upca)
endif ()
