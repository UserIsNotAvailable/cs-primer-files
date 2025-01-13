section .text
global fib
fib:
        mov     eax,    edi
        cmp     edi,    1
        jle     .return

        dec     edi
        push    rdi
        call    fib
        pop     rdi
        mov     ecx,    eax

        dec     edi
        push    rcx
        call    fib
        pop     rcx
        add     eax,    ecx
.return:
	    ret
