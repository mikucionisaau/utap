# Ensures that UTAP is installed

# The following will attempt to `find_package(UTAP 2.1.1)` and
# fall-back to download and install from github if not found.
include(FetchContent)
FetchContent_Declare(UTAP
        GIT_REPOSITORY https://github.com/UPPAALModelChecker/utap.git
        GIT_TAG v2.1.1      # fetches version 2.1.1, alternatively try 'main' branch
        GIT_SHALLOW TRUE    # get only the last commit version
        GIT_PROGRESS TRUE   # show progress of download
        EXCLUDE_FROM_ALL ON # don't build if not used
        FIND_PACKAGE_ARGS 2.1.1)

# UTAP build options:
set(UTAP_WARNINGS CACHE BOOL ON "UTAP Compiler Warnings")
set(UTAP_TESTS CACHE BOOL ON "UTAP Unit Tests")
set(UTAP_STATIC CACHE BOOL ON "UTAP Static Linking")
set(UTAP_CLANG_TIDY CACHE BOOL ON "Enable clang-tidy linting")
set(UTAP_CCACHE CACHE BOOL ON "Enables ccache for intermediate object file caching")

FetchContent_MakeAvailable(UTAP)

# Some diagnostics of what actually happened:
if (UTAP_FOUND)
    message(STATUS "Found UTAP: ${UTAP_INCLUDE_DIR}")
else (UTAP_FOUND)
    message(STATUS "Fetched UTAP: ${UTAP_SOURCE_DIR}")
endif (UTAP_FOUND)

# Some hints on what targets to link against:
if (TARGET UTAP::UTAP)
    message(STATUS "    Available target: UTAP::UTAP")
endif ()
if (TARGET UTAP)
    message(STATUS "    Available target: UTAP")
endif ()
