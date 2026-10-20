cmake_minimum_required(VERSION 3.16)
project(flageasy_consumer LANGUAGES CXX)

find_package(flageasy 1.0 REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE flageasy::flageasy)
