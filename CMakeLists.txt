cmake_minimum_required(VERSION 3.10)
project(FastTrack)

set (CMAKE_CXX_STANDARD 11)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost 1.71.0 REQUIRED COMPONENTS coroutine)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)
add_executable(FastTrack src/FastTrack.cpp src/HttpServer.cpp src/Session.cpp)
target_link_libraries(FastTrack PRIVATE ${Boost_LIBRARIES} Threads::Threads)
target_include_directories(FastTrack PUBLIC ${Boost_INCLUDE_DIRS})
