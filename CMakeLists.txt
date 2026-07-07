cmake_minimum_required(VERSION 3.16)
project(UTAP VERSION 2.1.1 LANGUAGES CXX C)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# in case of top level project:
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  message(STATUS "UTAP is the top-level project")
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      add_compile_options(-Wpedantic -Wall -Wextra -Werror=vla -Wno-unused-parameter)
  endif()
else()
  message(STATUS "UTAP is NOT the top-level project")
endif()

if(WIN32)
  set(UTAP_STATIC_DEFAULT ON)
  set(BUILD_SHARED_LIBS OFF)
else(WIN32)
  set(UTAP_STATIC_DEFAULT OFF)
endif(WIN32)

option(UTAP_WARNINGS "UTAP Compiler Warnings" ON)
option(UTAP_TESTS "UTAP Unit Tests" ON)
option(UTAP_STATIC "UTAP Static Linking" ${UTAP_STATIC_DEFAULT})
option(UTAP_CLANG_TIDY "Enable clang-tidy linting" ON)
option(UTAP_CCACHE "Enables ccache for intermediate object file caching" ON)

cmake_policy(SET CMP0048 NEW) # project() command manages VERSION variables
include(cmake/sanitizers.cmake)
if (UTAP_CLANG_TIDY)
    include(cmake/clang-tidy.cmake)
endif (UTAP_CLANG_TIDY)
if (UTAP_CCACHE)
    include(cmake/ccache.cmake)
endif (UTAP_CCACHE)

set(UTAP_PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(UTAP_VERSION "${PACKAGE_VERSION}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_CURRENT_SOURCE_DIR}/libs")

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.6.0 REQUIRED)
include(cmake/libxml2.cmake)

add_subdirectory(src)

if (UTAP_TESTS)
    include(cmake/doctest.cmake)
    enable_testing()
endif(UTAP_TESTS)
add_subdirectory(test)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/UTAPConfigVersion.cmake VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/utap
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS UTAP
        EXPORT UTAPConfig)
install(EXPORT UTAPConfig
        FILE UTAPConfig.cmake
        NAMESPACE UTAP::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UTAP-${PROJECT_VERSION})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/UTAPConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/UTAP-${PROJECT_VERSION})
