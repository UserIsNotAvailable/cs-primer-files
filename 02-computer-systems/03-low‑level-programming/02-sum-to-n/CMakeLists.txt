project(sum_to_n C ASM_NASM)

add_executable(sum_to_n
        sum_to_n_2.asm
        tests.c
)


#file(COPY  max_of_three.asm
#        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
