find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy-18 clang-tidy-16 clang-tidy-14 clang-tidy)
if(CLANG_TIDY_PROGRAM)
    execute_process(COMMAND ${CLANG_TIDY_PROGRAM} --version OUTPUT_VARIABLE CLANG_TIDY_VERSION_STRING)
    string(REGEX MATCH "LLVM version ([0-9A-Za-z]+)\\.([0-9A-Za-z\\.]+)" TIDY_VERSION "${CLANG_TIDY_VERSION_STRING}")
    set(TIDY_MAJOR_VERSION "${CMAKE_MATCH_1}")
    set(TIDY_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")

    # We use NOLINTBEGIN and NOLINTEND to preserve some macros
    # This was only introduced in version 14
    if (TIDY_MAJOR_VERSION GREATER_EQUAL 14)
        # Add good checks which produce a minimal amount of false positives
        # One or two false positives can be dealt with using // NOLINT
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PROGRAM})
        message(STATUS "Enabled clang-tidy ${TIDY_VERSION}: ${CLANG_TIDY_PROGRAM}")
    else()
        message(WARNING "Found clang-tidy ${TIDY_VERSION}, but >=14 is required, thus disabled.")
    endif()
else(CLANG_TIDY_PROGRAM)
    message(WARNING "No clang-tidy found. Some checks are not enabled")
endif(CLANG_TIDY_PROGRAM)
