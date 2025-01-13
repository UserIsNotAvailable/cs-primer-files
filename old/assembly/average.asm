            global      average
            extern      atoi
            extern      printf
            section     .text
average:
    ;           rdi, rsi, rdx, rcx, r12, r13.
    ;           rbp, rbx, r12, r13, r14, r15
            push        rbx
            push        r12
            push        r13
            xorpd       xmm0, xmm0
            dec         rdi
            jz          .done
            mov         [rel count], rdi
            mov         rbx, rdi
            mov         r12, rsi
.acc:
            mov         rdi, [r12 + rbx * 8]
            call        atoi
            add         [rel sum], rax
            dec         rbx
            jnz         .acc
.avg:
            cvtsi2sd    xmm0, [sum]
            cvtsi2sd    xmm1, [count]
            divsd       xmm0, xmm1
.done:
            mov         rax, 1
            lea         rdi, [rel format]
            call        printf
            pop         r13
            pop         r12
            pop         rbx
            ret
format:     db          "%f", 10, 0

            section     .bss
count:      resq        1

            section     .data
sum:        dq          0
