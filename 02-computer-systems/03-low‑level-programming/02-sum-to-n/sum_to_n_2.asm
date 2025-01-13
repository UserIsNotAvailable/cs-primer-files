section .text
global sum_to_n
sum_to_n:
;    xor     eax, eax
    lea     eax, [edi + 1]
    imul    eax, edi
    shr     eax, 1
	ret
