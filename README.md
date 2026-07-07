# libutap

Gerd Behrmann

Marius Mikučionis

1. What is libutap
2. Compiling
3. Simple use
4. Parsing trace files
5. Architecture
6. API Documentation
7. Issues

## 1. What is libutap

libutap is the Uppaal Timed Automata Parser. Uppaal is an integrated
tool environment for designing, simulating and verifying real time
systems modeled as extended timed automata. `libutap` is the parser for
Uppaal documents.

libutap has the ability to parse and type check Uppaal models in any
of the three file formats supported by Uppaal.

libutap is licensed under the LGPL.

## 2. Compiling

Use `cmake` to compile `libutap` easily for various platforms.
You will need GCC version 10 or newer, Ninja or GNU make,
and `libxml2` from [XMLSoft](https://www.xmlsoft.org) (at least version 2.6.10).

Install the developer tools:
```shell
sudo apt-get install g++ ninja-build cmake
```

Install the tools and libraries:
```shell
sudo apt-get install flex bison libxml2-dev doctest-dev
```

Configure UTAP:
```shell
cmake . -B build -DCMAKE_INSTALL_PREFIX=$MYPATH -G Ninja # to install to $MYPATH, e.g. /usr/local
```
Compile UTAP:
```shell
cmake --build build
```
Run unit tests:
```sh
ctest --test-dir build --output-on-failure
```

Install UTAP to `$MYPATH`:
```
cmake --install build
```

## Compile from scratch (almost)

Get library dependencies:
```shell
./getlibs.bash [platform]
```

Compile and test UTAP:
```shell
./compile.sh [platform]
```

## 3. Example Use Case

Directory [examples](examples) contains a self-contained example with build scripts with two ways of building using either:

* [compile-with-getlibs.sh](examples/compile-with-getlibs.sh) compiles and installs all the dependencies into [local](examples/local) directory and then builds the [example.cpp](examples/example.cpp) with it.
The script behavior can be customized by exporting the following environment variables:
    - `CMAKE_BUILD_TYPE` - specify the build type: `Debug`, `Release` (default), `RelWithDebInfo`, `MinSizeRel`.
    - `CMAKE_PREFIX_PATH` - specify where to look for libraries (default: `examples/local/$TARGET`).
    - `CMAKE_INSTALL_PREFIX` - specify where to instal the libraries (default: `examples/local/arch-kernel`).
    - `TARGET` - specify target architecture and kernel (default: native host), see [toolchain](cmake/toolchain) for a list of supported targets.
    - `UTAP_SRC` - specify the path to UTAP source directory (default this UTAP repository).
```shell
rm -Rf examples/build-*
examples/compile-with-getlibs.sh
```
 * [compile-with-cmake.sh](examples/compile-with-cmake.sh) uses CMake to get the dependencies and then builds [example.cpp](examples/example.cpp) with them. The benefit is that all dependencies can be build using the same options (debug/release/sanitizers etc).
```shell
rm -Rf examples/build-*
examples/compile-with-cmake.sh
```

## 4. Parsing Uppaal Trace Files

`verifyta` utility from Uppaal distribution (`bin-*` folder) is able to print traces in human readable format (see `verifyta -h` and in particular keys `-t` and `-f`, also `-X` can produce `xml`).

If a custom format or some kind of transformation is needed, UTAP includes `tracer` utility which can parse and print trace files in human readable form. The source is included, thus the output can be modified accordingly.

First, `tracer` needs an `.if` file -- an intermediate format representation of the same model that was used to produce the trace. The `.if` file is produced by `verifyta` utility from Uppaal distribution:
```sh
UPPAAL_COMPILE_ONLY=1 verifyta model.xml > model.if
```
Then `tracer` can be applied as follows:
```sh
tracer model.if trace.xtr
```

## 5. Architecture

The following ASCII figure shows the initial information flow through
the library.

<pre>
 +----\
 |    |\
 |    +-\
 |      |   --> libxml2 --[SAX-interface]--> "xmlreader.cc"
 | .xml |                                         |
 |      |                                         |
 +------+                                         |
                                                  |
 +----\                                           |
 |    |\                                          |
 |    +-\                                         v
 | .ta  |                            +--------------------------+
 | .xta |   -----------------------> | bison parser (parser.yy) |
 |      |                            +--------------------------+
 +------+                                           |
                                                    |
 +------+                                           |
 |      |                                           |
 | TAS  |                                           |
 |      |      +---------------+                    |
 |      | <--- | SystemBuilder |<--[ParserBuilder]--/
 |      |      +---------------+
 +------+
</pre>

The BNF implemented by the bison generated parser is split into
several sections and to some extend duplicated for the old and new
syntax. It can read `.ta` and `.xta` files directly. `XML` files are read
using the `libxml2` library. Each text-block in the `XML` file is then
parser wrt. the BNF for that block (see [xmlreader.cpp](src/xmlreader.cpp)). For this, the
`bison` generated parser is reused.

The parser calls methods in the abstract `ParserBuilder` class. The
methods in the `ParserBuilder` class are implemented by the
SystemBuilder class which is a subclass of ExpressionBuilder which is
a subclass of `AbstractBuilder`. The `SystemBuilder` writes the model to
an instance of the `TimedAutomataSystem` (TAS) class.

The design abstracts the differences between the `.xml`, `.xta` and `.ta`
input formats and also hides the differences between the 3.4 and
3.6/4.0 formats from any implementation of the `ParserBuilder` interface
(for equivalent input the parser will call the same methods in the
ParserBuilder class).

A TAS object represents the templates, variables, locations,
transitions and processes of a model. Symbols are represented by
`Symbol` objects (see the API documentation). A symbol is a name (a
string) with a type. The type is represented by a `Type`
object. Symbols are grouped into frames (represented by `Frame`
objects). Frames are used to represent scopes and other collections of
symbols such as records or parameters of templates and functions.

All expressions are represented using a tree structure where the
leaves represent values or variables and the inner nodes represent
operations. Each node is referenced using an `Expression` object.

## 6. API Documentation

[doxygen](https://www.doxygen.nl/) can be used to generate HTML documentation in `doc/api/html`:

1. Install doxygen:
```shell
sudo apt install doxygen
```
2. Generate and open the documentation:
```shell
cd doc/api
doxygen libutap.doxygen
xdg-open file://$PWD/html/index.html
```

For more options (LaTeX/pdf etc) use doxywizard (`sudo apt install doxygen-gui`).

## 7. Issues

Please use the [Issues](https://github.com/UPPAALModelChecker/utap/issues) tab at the top to report problems.
