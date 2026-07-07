# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING ON)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_FLAGS      -m32)
set(CMAKE_CXX_FLAGS    -m32)

# where is the target environment located
if (CMAKE_PREFIX_PATH)
    set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")
else(CMAKE_PREFIX_PATH)
    set(CMAKE_LIBRARY_PATH /usr/lib/i386-linux-gnu)
endif(CMAKE_PREFIX_PATH)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
