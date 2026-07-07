add_executable(pretty pretty.cpp)
target_link_libraries(pretty PRIVATE UTAP)

add_executable(syntaxcheck syntaxcheck.cpp)
target_link_libraries(syntaxcheck PRIVATE UTAP)

add_executable(featurecheck featurechecker.cpp)
target_link_libraries(featurecheck PRIVATE UTAP)

if (UTAP_STATIC)
  target_link_options(pretty PRIVATE -static)
  target_link_options(syntaxcheck PRIVATE -static)
  target_link_options(featurecheck PRIVATE -static)
endif (UTAP_STATIC)

install(TARGETS pretty syntaxcheck featurecheck)

if(UTAP_TESTS)
  #target_compile_definitions(MODELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models")
  add_executable(expression_test expression_test.cpp)
  target_link_libraries(expression_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME expression_test COMMAND expression_test)

  add_executable(statement_test statement_test.cpp)
  target_link_libraries(statement_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME statement_test COMMAND statement_test)

  add_library(external_fn SHARED external_fn.cpp)
  target_compile_features(external_fn PUBLIC cxx_std_11)

  add_executable(parser_test parser_test.cpp)
  target_compile_definitions(parser_test PRIVATE MODELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models")
  target_link_libraries(parser_test PRIVATE UTAP doctest::doctest_with_main)
  add_dependencies(parser_test external_fn)
  add_test(NAME parser_test COMMAND parser_test)

  add_executable(featurechecker_test featurechecker_test.cpp)
  target_compile_definitions(featurechecker_test PRIVATE MODELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models")
  target_link_libraries(featurechecker_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME featurechecker_test COMMAND featurechecker_test)

  add_executable(range_test range_test.cpp)
  target_link_libraries(range_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME range_test COMMAND range_test)

  add_executable(typechecker_test typechecker_test.cpp)
  target_compile_definitions(typechecker_test PRIVATE MODELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models")
  target_link_libraries(typechecker_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME typechecker_test COMMAND typechecker_test)

  add_executable(prettyprint_test prettyprint_test.cpp)
  target_compile_definitions(prettyprint_test PRIVATE MODELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models")
  target_link_libraries(prettyprint_test PRIVATE UTAP doctest::doctest_with_main)
  add_test(NAME prettyprint_test COMMAND prettyprint_test)

  add_executable(example_test example_test.cpp)
  target_link_libraries(example_test PRIVATE UTAP)
  add_test(NAME example_train_gate COMMAND example_test ${PROJECT_SOURCE_DIR}/examples/train-gate.xml ${PROJECT_SOURCE_DIR}/examples/train-gate-out.xml)

  if (UTAP_STATIC)
    target_link_options(expression_test PRIVATE -static)
    target_link_options(statement_test PRIVATE -static)
    target_link_options(parser_test PRIVATE -static)
    target_link_options(featurechecker_test PRIVATE -static)
    target_link_options(range_test PRIVATE -static)
    target_link_options(typechecker_test PRIVATE -static)
    target_link_options(prettyprint_test PRIVATE -static)
    target_link_options(example_test PRIVATE -static)
    message(STATUS "Enabled Unit Tests -static")
  endif (UTAP_STATIC)
endif(UTAP_TESTS)
