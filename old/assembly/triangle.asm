                global      _start
                section     .text
_start:
                mov         rax, buffer
                mov         rbx, 0d01
                xor         rcx, rcx
write_line:
                mov         byte[rax], '*'
                inc         rax
                inc         rcx
                cmp         rcx, rbx
                jl          write_line
line_end:
                mov         byte[rax], 0d10
                inc         rax
                xor         rcx, rcx
                inc         rbx
                cmp         rbx, max_lines
                jle         write_line
write_buffer:
                mov         rax, 0d01
                mov         rdi, 0d01
                mov         rsi, buffer
                mov         rdx, buffer_size
                syscall
                mov         rax, 0d60
                mov         rdi, 0d00
                syscall

                section     .bss
max_lines:      equ         0d10
buffer_size:    equ         0d65
buffer:         resb        buffer_size
