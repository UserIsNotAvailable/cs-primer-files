%define MASK 0x07fffffe
section .text
global pangram
pangram:
        xor     rax,    rax
        xor     rcx,    rcx
.loop:
        mov     cl,     [rdi]
        inc     rdi
        test    cl,     cl
        jz      .not_pangram
        cmp     cl,     '@'
        jle     .loop
;        and     cl,     0x1f
        bts     eax,    ecx
        and     eax,    MASK
        cmp     eax,    MASK
        jne     .loop
        mov     eax,    1
        ret
.not_pangram:
        xor     eax,    eax
        ret
