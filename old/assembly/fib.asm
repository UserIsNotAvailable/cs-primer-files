        global      main
        extern      printf

        section .text
main:
        push    rbx
        mov     rsi, 0
        xor     rdx, rdx
        mov     rbx, 1
print:
        push    rsi
        push    rdx
        xor     rax, rax
        mov     rdi, format
        call    printf
        pop     rdx
        pop     rsi
print_done:
        mov     r8, rdx
        mov     rdx, rbx
        add     rbx, r8
        inc     rsi
        cmp     rsi, max
        jl      print
        pop     rbx
        ret

        section .data
max     equ     20
format: db      "%d%10d", 10, 0







