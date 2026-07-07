set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_VERSION 10.15)

# cross compilers to use for C, C++
set(CMAKE_C_COMPILER   gcc-13)
set(CMAKE_CXX_COMPILER g++-13)
set(CMAKE_AR           gcc-ar-13)
set(CMAKE_NM           gcc-nm-13)
set(CMAKE_RANLIB       gcc-ranlib-13)
set(RANLIB             gcc-ranlib-13)
set(CMAKE_STRIP        strip)

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
