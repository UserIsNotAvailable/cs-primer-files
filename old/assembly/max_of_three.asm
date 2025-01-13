        global      max_of_three

        section     .text
max_of_three:
        mov         rax, rdi
        cmp         rax, rsi
        cmovl       rax, rsi
        cmp         rax, rdx
        cmovl       rax, rdx
        ret
