cmake_minimum_required(VERSION 4.0)
project(display_RYB C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(time_test_version)
include_directories(/usr/local/include/libpynq/library)

add_executable(display_RYB
        time_test_version/speed-file.c
        time_test_version/speed-pipe.c
        GenerateRawFiles.c
        main.c
        fonts/font-test.c
        disply-test-on-cli/clitest.c
        calloc_version/clitest_v2.c
        old/main.old.c)
