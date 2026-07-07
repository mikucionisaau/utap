# Ensures that LibXml2 is installed
include(FetchContent)

FetchContent_Declare(LibXml2
    GIT_REPOSITORY https://github.com/GNOME/libxml2.git # mirror
    #GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2 # often down
    GIT_TAG v2.13.9
    #URL https://download.gnome.org/sources/libxml2/2.13/libxml2-2.13.9.tar.xz
    #URL_HASH SHA256=7ce458a0affeb83f0b55f1f4f9e0e55735dbfc1a9de124ee86fb4a66b597203a
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    EXCLUDE_FROM_ALL ON # don't build if not used
    FIND_PACKAGE_ARGS 2.9.14)

set(LIBXML2_SHARED_LIBS OFF CACHE BOOL "LibXml2 shared libraries")
set(LIBXML2_WITH_FTP OFF CACHE BOOL "LibXml2 FTP support")
set(LIBXML2_WITH_HTTP OFF CACHE BOOL "LibXml2 HTTP support")
set(LIBXML2_WITH_ICONV OFF CACHE BOOL "LibXml2 ICONV")
set(LIBXML2_WITH_ICU OFF CACHE BOOL "LibXml2 ICU")
set(LIBXML2_WITH_LZMA OFF CACHE BOOL "LibXml2 LZMA")
set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "LibXml2 Python")
set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "LibXml2 libz")
set(LIBXML2_WITH_THREADS OFF CACHE BOOL "LibXml2 multithread support")
set(LIBXML2_WITH_TESTS OFF CACHE BOOL "LibXml2 Tests")

FetchContent_MakeAvailable(LibXml2)

if(LibXml2_FOUND) # find_package
    message(STATUS "Found LibXml2: ${LIBXML2_INCLUDE_DIR}")
else(LibXml2_FOUND) # FetchContent
    message(STATUS "Fetched LibXml2: ${libxml2_SOURCE_DIR}")
endif(LibXml2_FOUND)

if (TARGET LibXml2::LibXml2)
    message(STATUS "    Available target: LibXml2::LibXml2")
endif ()

if (TARGET LibXml2)
    message(STATUS "    Available target: LibXml2")
endif ()
