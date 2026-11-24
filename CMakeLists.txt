# project setup and some options
cmake_minimum_required(VERSION 3.20)
project(twostream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(TS_ENABLE_MPI "Build the OpenMP + MPI version (Talapas only)" OFF)
option(TS_ENABLE_GPU "Build the CUDA version (Talapas only)" OFF)

# shared parts
add_library(core STATIC particle.cpp sim.cpp)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(core PUBLIC _USE_MATH_DEFINES)

# serial
add_executable(ts_serial main_serial.cpp)
target_link_libraries(ts_serial PRIVATE core)

# checks against the paper and known physics
add_executable(ts_verify main_verify.cpp)
target_link_libraries(ts_verify PRIVATE core)

# openmp
find_package(OpenMP COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
  add_executable(ts_omp main_omp.cpp)
  target_link_libraries(ts_omp PRIVATE core OpenMP::OpenMP_CXX)
else()
  message(WARNING "OpenMP not found, skipping ts_omp")
endif()

# openmpi
if(TS_ENABLE_MPI)
  if(NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "TS_ENABLE_MPI needs OpenMP")
  endif()
  find_package(MPI REQUIRED COMPONENTS CXX)
  add_executable(ts_hybrid main_hybrid.cpp)
  target_link_libraries(ts_hybrid PRIVATE core OpenMP::OpenMP_CXX MPI::MPI_CXX)
endif()

# gpu
if(TS_ENABLE_GPU)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 80)
  endif()
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
  add_executable(ts_gpu main_gpu.cu)
  target_link_libraries(ts_gpu PRIVATE core)
endif()
