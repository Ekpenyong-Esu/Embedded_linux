cmake_minimum_required(VERSION 3.16.3)
project(aesd-assignments)

set(CMAKE_C_FLAGS "-pthread")

set(AUTOTEST_SOURCES
    test/assignment1/Test_hello.c
    test/assignment1/Test_assignment_validate.c
    test/assignment7/Test_circular_buffer.c
)

# A list of all files containing test code that is used for assignment validation
set(TESTED_SOURCE
    ../examples/autotest-validate/autotest-validate.c
    ../aesd-char-driver/circular-buffer/src/aesd-circular-buffer-add.c
    ../aesd-char-driver/circular-buffer/src/aesd-circular-buffer-remove.c
    ../aesd-char-driver/circular-buffer/src/aesd-circular-buffer-init.c
    ../aesd-char-driver/circular-buffer/src/aesd-circular-buffer-find.c
)

add_subdirectory(assignment-autotest)
