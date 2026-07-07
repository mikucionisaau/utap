#!/usr/bin/env bash
set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

LOCAL="$PROJECT_DIR/local"
SOURCE="$LOCAL/source"
BW="\e[1;97m" # bold white
NC="\e[0m"    # no color

LIBXML2=libxml2-2.13.9
LIBXML2_Z="${LIBXML2}.tar.xz"
LIBXML2_URL="https://people.cs.aau.dk/~marius/mirrors/libxml2/${LIBXML2_Z}"
LIBXML2_SHA256=a2c9ae7b770da34860050c309f903221c67830c86e4a7e760692b803df95143a


DOCTEST=doctest-2.4.12
DOCTEST_Z="${DOCTEST}.tar.gz"
DOCTEST_URL="https://github.com/doctest/doctest/archive/refs/tags/v${DOCTEST#doctest-}.tar.gz"
DOCTEST_SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70

BISON=bison-3.8.2
BISON_Z="${BISON}.tar.xz"
BISON_URL="https://ftp.gnu.org/gnu/bison/${BISON_Z}"
BISON_SHA256=9bba0214ccf7f1079c5d59210045227bcf619519840ebfa80cd3849cff5a5bf2

function download_unpack {
    local LIB="${1}"
    local LIB_URL="${LIB}_URL"
    local LIB_SUM="${LIB}_SHA256"
    local LIB_Z="${LIB}_Z"
    mkdir -p "$SOURCE"
    pushd "$SOURCE"
    if [ ! -r "${!LIB_Z}" ]; then
        echo -e "${BW}Downloading ${!LIB_Z}${NC}"
        wget -nv "${!LIB_URL}" -O "${!LIB_Z}" || { echo "Download failed"; exit 1; }
    fi
    echo "${!LIB_SUM}  ${!LIB_Z}" | shasum -a256 --check - || \
        { echo "  expected: ${!LIB_SUM}"; echo "  got     : $(shasum -a256 "${!LIB_Z}")"; exit 2; }
    if [ ! -d "${!LIB}" ]; then
        echo -e "${BW}Extracting ${!LIB}${NC}"
        case "${!LIB_Z}" in
            *.7z)
                7z x -aoa "${!LIB_Z}"
                ;;
            *.tar.*)
                tar xf "${!LIB_Z}"
                ;;
            *)
                echo "Don't know how to extract ${!LIB_Z}"
                exit 1
        esac
    fi
    popd
}

if [ "$#" -lt 1 ]; then
    echo "Expecting a list of target platforms as arguments."
    echo -e "For example: ${BW}$0 x86_64-linux x86_64-w64-mingw32 darwin-brew-gcc14${NC}"
    echo -e "List of supported platforms:"
    for  toolchain in "${PROJECT_DIR}/cmake/toolchain"/*.cmake ; do
        platform=$(basename "$toolchain")
        echo -e "    ${BW}${platform%%.cmake}${NC}"
    done
fi

missing_tools=""
for tool in wget tar shasum cmake ; do
    if [ -z "$(command -v $tool)" ] ; then
        echo "Could not find $tool"
        missing_tools="${missing_tools:+$missing_tools} $tool"
    fi
done
if [ -n "$missing_tools" ]; then
    exit 1
fi

for target in "$@" ; do
    if [ -n "$CMAKE_INSTALL_PREFIX" ] ; then
      LIBS="$CMAKE_INSTALL_PREFIX"
    else
      LIBS="$LOCAL/$target"
    fi
    # libxml2
    if [ ! -r "$LIBS/lib/libxml2.a" ] ; then
        echo -e "${BW}Preparing source of ${LIBXML2}${NC}"
        download_unpack LIBXML2
        BUILD="$LIBS/build-${LIBXML2}"
        echo -e "${BW}${target}: Configuring ${LIBXML2}${NC}"
        cmake -S "$SOURCE/$LIBXML2" -B "$BUILD" \
          -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/${target}.cmake" \
          -DCMAKE_PREFIX_PATH="$LIBS" -DCMAKE_INSTALL_PREFIX="$LIBS" -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=OFF -DLIBXML2_WITH_C14N=OFF -DLIBXML2_WITH_CATALOG=OFF \
          -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_FTP=OFF -DLIBXML2_WITH_HTTP=OFF \
          -DLIBXML2_WITH_HTML=OFF -DLIBXML2_WITH_ICONV=OFF -DLIBXML2_WITH_ICU=OFF -DLIBXML2_WITH_ISO8859X=ON \
          -DLIBXML2_WITH_LZMA=OFF -DLIBXML2_WITH_MODULES=OFF \
          -DLIBXML2_WITH_OUTPUT=ON -DLIBXML2_WITH_PATTERN=OFF -DLIBXML2_WITH_PROGRAMS=OFF \
          -DLIBXML2_WITH_PUSH=ON -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_READER=ON \
          -DLIBXML2_WITH_REGEXPS=OFF -DLIBXML2_WITH_SAX1=OFF \
          -DLIBXML2_WITH_SCHEMAS=OFF -DLIBXML2_WITH_SCHEMATRON=OFF -DLIBXML2_WITH_TESTS=OFF \
          -DLIBXML2_WITH_THREADS=OFF -DLIBXML2_WITH_THREAD_ALLOC=OFF -DLIBXML2_WITH_TLS=OFF \
          -DLIBXML2_WITH_TREE=OFF -DLIBXML2_WITH_VALID=OFF -DLIBXML2_WITH_WRITER=ON \
          -DLIBXML2_WITH_XINCLUDE=OFF -DLIBXML2_WITH_XPATH=ON \
          -DLIBXML2_WITH_XPTR=OFF -DLIBXML2_WITH_XPTR_LOCS=OFF \
          -DLIBXML2_WITH_ZLIB=OFF
        echo -e "${BW}${target}: Building ${LIBXML2}${NC}"
        cmake --build "$BUILD"
        #echo -e "${BW}${target}: Testing ${LIBXML2}${NC}"
        #ctest --test-dir "$BUILD" --output-on-failure
        echo -e "${BW}${target}: Installing ${LIBXML2}${NC}"
        cmake --install "$BUILD" --prefix="$LIBS" --config Release
        rm -Rf "$BUILD"
        echo -e "${BW}${target}: Success ${LIBXML2}${NC}"
    else
        echo -e "${BW}${target}: ${LIBXML2} is already installed.${NC}"
    fi
    if [ ! -r "$LIBS/include/doctest/doctest.h" ] ; then
        echo -e "${BW}Preparing source of ${DOCTEST}${NC}"
        download_unpack DOCTEST
        BUILD="$LIBS/build-${DOCTEST}"
        echo -e "${BW}${target}: Configuring ${DOCTEST}${NC}"
        cmake -S "$SOURCE/$DOCTEST" -B "$BUILD" -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain/${target}.cmake" \
          -DCMAKE_PREFIX_PATH="$LIBS" -DCMAKE_INSTALL_PREFIX="$LIBS" -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
          -DDOCTEST_WITH_TESTS=OFF -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=ON -DDOCTEST_USE_STD_HEADERS=OFF
        echo -e "${BW}${target}: Building ${DOCTEST}${NC}"
        cmake --build "$BUILD"
        #echo -e "${BW}${target}: Testing ${DOCTEST}${NC}"
        #ctest --test-dir "$BUILD" --output-on-failure
        echo -e "${BW}${target}: Installing ${DOCTEST}${NC}"
        cmake --install "$BUILD" --prefix="$LIBS" --config Release
        rm -Rf "$BUILD"
        echo -e "${BW}${target}: Success ${DOCTEST}${NC}"
    else
        echo -e "${BW}${target}: ${DOCTEST} is already installed.${NC}"
    fi
    if [ -n "$(command -v bison)" ]; then
        echo -e "${BW}${target}: ${BISON} is provided by OS.${NC}"
    elif [ -x "$LIBS/bin/bison" ] ; then
        echo -e "${BW}${target}: ${BISON} is already installed.${NC}"
    else
        echo -e "${BW}Preparing source of ${BISON}${NC}"
        download_unpack BISON
        pushd "$SOURCE/$BISON/"
        autoconf
        popd
        echo -e "${BW}${target}: Configuring ${BISON}${NC}"
        BUILD="$LIBS/build-${BISON}"
        mkdir -p "$BUILD"
        pushd "$BUILD"
        "$SOURCE/$BISON/configure" --prefix="$LIBS"
        echo -e "${BW}${target}: Building ${BISON}${NC}"
        make -j$(nproc)
        echo -e "${BW}${target}: Installing ${BISON}${NC}"
        make install
        popd
        rm -Rf "$BUILD"
        echo -e "${BW}${target}: Success ${BISON}${NC}"
    fi
done
