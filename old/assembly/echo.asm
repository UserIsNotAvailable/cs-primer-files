            global      echo
            extern      puts

            section     .text
echo:
;           rdi, rsi, rdx, rcx, r8, r9.
;           rbp, rbx, r12, r13, r14, r15
.start:
            push        rbx
            mov         rbx, rdi
.loop:
            mov         rdi, [rsi]
            push        rsi
            call        puts
            pop         rsi
            add         rsi, 8
            dec         rbx
            jnz         .loop
.done:
            pop         rbx
            ret
