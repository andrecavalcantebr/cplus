cmake_minimum_required(VERSION 3.21)

project(cplus VERSION 0.1.0 LANGUAGES C)

# Build options
option(CPLUS_BUILD_TESTS "Build tests" ON)
option(CPLUS_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# Export compile commands for IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF) # avoid gnu23 extensions

option(CPLUS_ENABLE_SANITIZERS "Enable runtime sanitizers in Debug" ON)

function(cplus_apply_warnings target_name)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wformat=2
            -Warray-bounds=2
            -Wstringop-overflow=4
            -Wstringop-truncation
            -Wformat-overflow=2
            -Wformat-truncation=2
            -Walloc-size-larger-than=1048576
            -Wvla
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=3
        )
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wformat=2
            -Warray-bounds
            -Wvla
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=3
        )
    endif()

    if(CPLUS_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()

# Collect source files
file(GLOB_RECURSE CPLUS_ALL_SRC CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

# Core library = all src except main.c
set(CPLUS_CORE_SRC ${CPLUS_ALL_SRC})
list(FILTER CPLUS_CORE_SRC EXCLUDE REGEX ".*/main\\.c$")

add_library(cplus_core STATIC ${CPLUS_CORE_SRC})
target_include_directories(cplus_core
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/inc"
        "${CMAKE_SOURCE_DIR}/vendor"
)
cplus_apply_warnings(cplus_core)

# Main executable
add_executable(cplus "${CMAKE_SOURCE_DIR}/src/main.c")
target_link_libraries(cplus PRIVATE cplus_core)
target_include_directories(cplus PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/inc"
    "${CMAKE_SOURCE_DIR}/vendor"
)
cplus_apply_warnings(cplus)

# Tests
if(CPLUS_BUILD_TESTS)
    enable_testing()

    file(GLOB_RECURSE CPLUS_TEST_SRC CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/tests/*.c"
    )

    foreach(test_src IN LISTS CPLUS_TEST_SRC)
        get_filename_component(test_name "${test_src}" NAME_WE)

        add_executable("${test_name}" "${test_src}")
        target_link_libraries("${test_name}" PRIVATE cplus_core)
        target_include_directories("${test_name}" PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/inc"
            "${CMAKE_SOURCE_DIR}/vendor"
        )
        cplus_apply_warnings("${test_name}")

        add_test(NAME "${test_name}" COMMAND "${test_name}")
    endforeach()
endif()

# Linker libs requested by project policy
target_link_libraries(cplus PRIVATE cplus_core m pthread)

# If you create test executables, also link:
# target_link_libraries(<test_target> PRIVATE cplus_core m pthread)

include(CTest)

find_program(VALGRIND_EXECUTABLE valgrind)

if(VALGRIND_EXECUTABLE)
    add_custom_target(memcheck
        COMMAND ${CMAKE_CTEST_COMMAND}
            --test-dir ${CMAKE_BINARY_DIR}
            --output-on-failure
            -T memcheck
        COMMENT "Running tests under Valgrind"
    )

    set(MEMORYCHECK_COMMAND ${VALGRIND_EXECUTABLE})
    set(MEMORYCHECK_COMMAND_OPTIONS
        "--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
    )
endif()