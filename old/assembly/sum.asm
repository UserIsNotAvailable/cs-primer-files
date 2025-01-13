            global      sum
            section     .text
sum:
            xorpd       xmm0, xmm0
.next:
            test        rsi, rsi
            jz          .done
            addsd       xmm0, [rdi+(rsi-1)*8]
            dec         rsi
            jmp         .next
.done:
            ret