section .text
global binary_convert
binary_convert:
        xor     eax,    eax
        xor     ecx,    ecx
.next:
        mov     cl,     [rdi]
        test    cl,     cl
        jz      .done
        and     cl,     0x01
        shl     eax,    0x01
        or      eax,    ecx
        inc     rdi
        jmp     .next
.done:
	    ret
