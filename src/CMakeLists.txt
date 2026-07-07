# Create directory for the generated header files:
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/utap")

set(lexer_source "${CMAKE_CURRENT_BINARY_DIR}/lexer.cc")
add_custom_command(
  OUTPUT "${lexer_source}"
  COMMAND ${FLEX_EXECUTABLE} "--outfile=${lexer_source}" -Putap_
          "${CMAKE_CURRENT_SOURCE_DIR}/lexer.l"
  DEPENDS lexer.l)
add_custom_target(lexer_generate DEPENDS "${lexer_source}")

set(parser_source "${CMAKE_CURRENT_BINARY_DIR}/parser.cpp")
add_custom_command(
  OUTPUT "${parser_source}" "${CMAKE_CURRENT_BINARY_DIR}/include/parser.hpp"
  COMMAND
    ${BISON_EXECUTABLE} -putap_ -bparser "${CMAKE_CURRENT_SOURCE_DIR}/parser.y"
    "--output=${parser_source}"
    "--defines=${CMAKE_CURRENT_BINARY_DIR}/include/parser.hpp"
  DEPENDS lexer_generate parser.y)
add_custom_target(parser_generate DEPENDS "${parser_source}")

add_library(UTAP
  AbstractBuilder.cpp
  DocumentBuilder.cpp
  ExpressionBuilder.cpp
  FeatureChecker.cpp
  Library.cpp
  PrettyPrinter.cpp
  StatementBuilder.cpp
  TypeChecker.cpp
  document.cpp
  expression.cpp
  keywords.cpp
  position.cpp
  property.cpp
  statement.cpp
  symbols.cpp
  type.cpp
  typeexception.cpp
  xmlreader.cpp
  xmlwriter.cpp
  ${parser_source})
target_compile_features(UTAP PUBLIC cxx_std_17)
target_compile_definitions(UTAP PRIVATE _USE_MATH_DEFINES) # M_PI in xmlwriter.cpp
if (UTAP_STATIC)
  target_compile_definitions(UTAP PUBLIC LIBXML_STATIC)
  target_link_options(UTAP PUBLIC -static)
  message(STATUS "Enabled LIBXML_STATIC -static")
endif (UTAP_STATIC)
set_target_properties(UTAP PROPERTIES
                           CXX_STANDARD_REQUIRED ON
                           CXX_STANDARD_EXTENSIONS OFF
                           POSITION_INDEPENDENT_CODE ON)
if (UTAP_WARNINGS)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(UTAP PUBLIC -Wpedantic -Wall -Wextra -Wconversion -Wno-sign-conversion -Wimplicit-fallthrough)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(UTAP PUBLIC -Wpedantic -Wall -Wextra -Wconversion -Wno-sign-conversion -Wimplicit-fallthrough)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(UTAP PUBLIC -Wpedantic -Wall -Wextra -Wimplicit-fallthrough)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(UTAP PUBLIC /Wall)
  else ()
    message(WARNING "Failed to enable warnings for ${CMAKE_CXX_COMPILER_ID}")
  endif ()
endif ()
target_include_directories(UTAP
        PRIVATE
          # where the library itself will look for its internal headers
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/include # generated parser headers
        PUBLIC
          # where top-level project will look for the library's public headers
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
          # where external projects will look for the library's public headers
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(UTAP PRIVATE LibXml2::LibXml2 ${CMAKE_DL_LIBS}
                                   ${LIBXML_WINLIBS})
add_library(UTAP::UTAP ALIAS UTAP) # in case the library is not exported-imported
