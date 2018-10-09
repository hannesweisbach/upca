if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(UPCA_SRC arch/x86_64.cc)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  set(UPCA_SRC arch/aarch64.cc)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
  set(UPCA_SRC arch/aarch32.cc)
elseif(CMAKE_SYSTEM_NAME MATCHES "BlueGene")
  set(UPCA_SRC arch/bgq.cc)
endif()


add_library(upca ${UPCA_SRC})

# add include folders to the library and targets that consume it
target_include_directories(upca INTERFACE
  $<BUILD_INTERFACE: ${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE: ${PROJECT_BINARY_DIR}>
)
target_link_libraries(upca PUBLIC GSL)

if(JEVENTS_FOUND)
  target_link_libraries(upca PUBLIC jevents)
elseif(BGPM_FOUND)
  target_link_libraries(upca PUBLIC bgpm)
endif()

