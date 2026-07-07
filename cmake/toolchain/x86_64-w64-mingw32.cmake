# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

if(DEFINED ENV{CROSSCOMPILING_EMULATOR})
    set(CMAKE_CROSSCOMPILING_EMULATOR $ENV{CROSSCOMPILING_EMULATOR})
endif()

set(TARGET_PREFIX x86_64-w64-mingw32)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   ${TARGET_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TARGET_PREFIX}-g++)

# where is the target environment located
set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}")

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Workaround for "libxml2.a(dict.c.obj):dict.c:(.text+0x1226): undefined reference to `BCryptGenRandom'"
set(LIBXML_WINLIBS bcrypt)
