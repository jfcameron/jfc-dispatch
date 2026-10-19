# © Joseph Cameron - All Rights Reserved

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(jfcdispatch
    VERSION 0.1
    DESCRIPTION "how a caller lends a library its threads: one dispatch vocabulary, and code-style §7.1 kept once"
    LANGUAGES CXX)

if (TARGET jfcdispatch)
    return()
endif()

include(GNUInstallDirs)

option(JFCDISPATCH_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(JFCDISPATCH_BUILD_DOCS "Build documentation" OFF)

if (JFCDISPATCH_BUILD_TESTS OR JFCDISPATCH_BUILD_DOCS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/jfc-cmake/jfclib.cmake")
endif()

add_library(jfcdispatch INTERFACE)

add_library(jfc::dispatch ALIAS jfcdispatch)

set_target_properties(jfcdispatch PROPERTIES EXPORT_NAME dispatch)

target_include_directories(jfcdispatch INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_compile_features(jfcdispatch INTERFACE cxx_std_20)

include(CMakePackageConfigHelpers)

install(TARGETS jfcdispatch EXPORT jfcdispatch-targets)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT jfcdispatch-targets
    FILE jfcdispatch-targets.cmake
    NAMESPACE jfc::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jfcdispatch")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/jfcdispatch-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

file(WRITE "${PROJECT_BINARY_DIR}/jfcdispatch-config.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/jfcdispatch-targets.cmake\")
")

install(FILES
    "${PROJECT_BINARY_DIR}/jfcdispatch-config.cmake"
    "${PROJECT_BINARY_DIR}/jfcdispatch-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jfcdispatch")

if (JFCDISPATCH_BUILD_TESTS)
    add_subdirectory(test)
endif()

if (JFCDISPATCH_BUILD_DOCS)
    add_subdirectory(docs)
endif()

