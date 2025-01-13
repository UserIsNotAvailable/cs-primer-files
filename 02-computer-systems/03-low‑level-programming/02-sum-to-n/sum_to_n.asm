section .text
global sum_to_n
sum_to_n:
    xor     eax, eax
.loop:
    add     eax, edi
    dec     edi
    jg     .loop
.done:
	ret
