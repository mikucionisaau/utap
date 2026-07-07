cmake_minimum_required(VERSION 3.22)
project(UTAPExample CXX)

include(cmake/libxml2.cmake)
include(cmake/UTAP.cmake)

enable_testing()

# The actual usage example:
add_executable(example example.cpp)
target_link_libraries(example PRIVATE UTAP::UTAP)
add_test(NAME example COMMAND example)
set_tests_properties(example PROPERTIES WILL_FAIL TRUE)
add_test(NAME example_train_gate
        COMMAND example "${PROJECT_SOURCE_DIR}/train-gate.xml" "${PROJECT_SOURCE_DIR}/train-gate-out.xml")
#add_test(NAME example_compare
#        COMMAND diff "${PROJECT_SOURCE_DIR}/train-gate.xml" "${PROJECT_SOURCE_DIR}/train-gate-out.xml")
#set_tests_properties(example_compare PROPERTIES DEPENDS example_train_gate)
