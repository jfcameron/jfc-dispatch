# © Joseph Cameron - MIT License

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

if (TARGET jfc::dispatch)
    return()
endif()

project(jfcdispatch
    VERSION 1.0.0
    DESCRIPTION "lets a caller lend a library its threads, and runs serially when it lends none"
    LANGUAGES CXX)

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

install(TARGETS jfcdispatch EXPORT jfc-dispatch-targets)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT jfc-dispatch-targets
    FILE jfc-dispatch-targets.cmake
    NAMESPACE jfc::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jfc-dispatch")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/jfc-dispatch-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

file(WRITE "${PROJECT_BINARY_DIR}/jfc-dispatch-config.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/jfc-dispatch-targets.cmake\")
")

install(FILES
    "${PROJECT_BINARY_DIR}/jfc-dispatch-config.cmake"
    "${PROJECT_BINARY_DIR}/jfc-dispatch-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/jfc-dispatch")

if (JFCDISPATCH_BUILD_TESTS)
    add_subdirectory(test)
endif()

if (JFCDISPATCH_BUILD_DOCS)
    jfc_generate_documentation_doxygen(
        README_PATH      README.md
        OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs

        INCLUDE_DIRECTORIES
            include/
    )
endif()

