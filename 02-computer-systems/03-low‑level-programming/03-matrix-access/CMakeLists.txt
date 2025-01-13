project(matrix C ASM_NASM)

add_executable(matrix
        matrix.asm
        tests.c
)


#file(COPY  max_of_three.asm
#        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
