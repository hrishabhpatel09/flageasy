add_executable(flageasy_demo demo.cpp)
target_link_libraries(flageasy_demo PRIVATE flageasy::flageasy)

if(NOT MSVC)
    target_compile_options(flageasy_demo PRIVATE -Wall -Wextra)
endif()
