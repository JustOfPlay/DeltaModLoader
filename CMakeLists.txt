cmake_minimum_required(VERSION 3.26)
project(DeltaModLoader)

set(CMAKE_CXX_STANDARD 17)

add_executable(DeltaModLoader main.cpp

)
# Include the necessary standard libraries
find_package(Threads)
target_link_libraries(DeltaModLoader ${CMAKE_THREAD_LIBS_INIT})
include_directories(libs)

