cmake_minimum_required(VERSION 3.20)
project(pic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(PIC_ENABLE_GPU "Build the GPU version (supercomputer only)" OFF)

add_library(core STATIC particle.cpp)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(core PUBLIC _USE_MATH_DEFINES)

add_executable(pic_serial main_serial.cpp)
target_link_libraries(pic_serial PRIVATE core)

find_package(OpenMP COMPONENTS CXX)
find_package(MPI COMPONENTS CXX)
if(OpenMP_CXX_FOUND AND MPI_CXX_FOUND)
  add_executable(pic_hybrid main_hybrid.cpp)
  target_link_libraries(pic_hybrid PRIVATE core OpenMP::OpenMP_CXX MPI::MPI_CXX)
else()
  message(WARNING "OpenMP and MPI are both required, skipping pic_hybrid")
endif()

if(PIC_ENABLE_GPU)
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
  add_executable(pic_gpu main_gpu.cu)
  target_link_libraries(pic_gpu PRIVATE core)
endif()
