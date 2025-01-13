            global      factorial

            section     .text
factorial:
            cmp         rdi, 1
            jg          .L1
            mov         rax, 1
            ret
.L1:
            push        rdi
            dec         rdi
            call        factorial
            pop         rdi
            imul        rax, rdi
            ret
