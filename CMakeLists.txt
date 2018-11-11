
cmake_minimum_required(VERSION 3.11)
project(handle_failure)
set(CMAKE_CXX_STANDARD 17)
enable_testing()

add_library               (handle_failure INTERFACE)
target_include_directories(handle_failure INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable       (tests "tests/main.cpp")
target_link_libraries(tests handle_failure)
add_test        (NAME tests COMMAND tests)

if (MSVC)
    add_compile_options(tests "/W4 /WX")
else ()
    add_compile_options(tests "-Wall" "-Wextra" "-Werror")
endif()
