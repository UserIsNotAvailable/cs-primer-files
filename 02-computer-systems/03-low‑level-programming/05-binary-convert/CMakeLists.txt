project(binary_convert C ASM_NASM)

add_executable(binary_convert
        tests.c
        binary_convert.asm
)

