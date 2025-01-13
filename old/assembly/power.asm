            global      power

            extern      atoi
            extern      printf
            extern      puts
            section     .text
power:
    ;           rdi, rsi, rdx, rcx, r12, r13.
    ;           rbp, rbx, r12, r13, r14, r15
            push        rbx
            push        r12
            push        r13

            cmp         rdi, 3
            jne         .err_1

            mov         r13, rsi
            mov         rdi, [r13+16]
            call        atoi
            cmp         eax, 0
            jl          .err_2
            mov         r12d, eax

            mov         rdi, [r13+8]
            call        atoi
            mov         ebx, eax

            mov         r13, 1
.loop:
            test        r12, r12
            jz          .print_ret
            imul        r13d, ebx
            dec         r12d
            jmp         .loop
.print_ret:
            xor         eax, eax
            mov         rdi, format
            movsxd      rsi, r13d
            call        printf
            jmp         .done
.err_1:
            mov         rdi, err_msg_1
            jmp         .print_err
.err_2:
            mov         rdi, err_msg_2
.print_err:
            call        puts
            jmp         .done
.done:
            pop         r13
            pop         r12
            pop         rbx
            ret
            section     .data
format:     db          "%d", 10, 0
err_msg_1:  db          "2 args!", 0
err_msg_2:  db          "exponent no negative!", 0
