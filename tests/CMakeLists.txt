add_executable(flageasy_tests
    test_main.cpp
    test_basics.cpp
    test_options.cpp
    test_validation.cpp
)
target_link_libraries(flageasy_tests PRIVATE flageasy::flageasy)

if(NOT MSVC)
    target_compile_options(flageasy_tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME flageasy_tests COMMAND flageasy_tests)
