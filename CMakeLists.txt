cmake_minimum_required(VERSION 3.16)
project(flageasy VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(FLAGEASY_PUBLIC_HEADERS
    include/flageasy/flageasy.hpp
)

add_library(flageasy SHARED
    src/common.cpp
    src/convert.cpp
    src/flag.cpp
    src/help.cpp
    src/parser.cpp
    src/result.cpp
)
add_library(flageasy::flageasy ALIAS flageasy)

target_compile_features(flageasy PUBLIC cxx_std_17)
target_include_directories(flageasy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(flageasy PRIVATE FLAGEASY_BUILDING)

set_target_properties(flageasy PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INSTALL_NAME_DIR "@rpath"
)

if(NOT MSVC)
    target_compile_options(flageasy PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Install + package export (find_package(flageasy))
# ---------------------------------------------------------------------------
install(TARGETS flageasy
    EXPORT flageasyTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${FLAGEASY_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flageasy
)
install(EXPORT flageasyTargets
    NAMESPACE flageasy::
    FILE flageasyTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flageasy
)

configure_package_config_file(
    cmake/flageasyConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/flageasyConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flageasy
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/flageasyConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/flageasyConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/flageasyConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flageasy
)

# ---------------------------------------------------------------------------
# Tests and examples (only by default when built as the top-level project)
# ---------------------------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(FLAGEASY_IS_TOP_LEVEL ON)
else()
    set(FLAGEASY_IS_TOP_LEVEL OFF)
endif()

option(FLAGEASY_BUILD_TESTS "Build flageasy tests" ${FLAGEASY_IS_TOP_LEVEL})
option(FLAGEASY_BUILD_EXAMPLES "Build flageasy examples" ${FLAGEASY_IS_TOP_LEVEL})

if(FLAGEASY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(FLAGEASY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
