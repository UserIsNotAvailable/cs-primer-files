section .text
global index
index:
	; rdi: matrix
	; esi: rows
	; edx: cols
	; ecx: rindex
	; r8d: cindex
    imul    edx, ecx
    add     edx, r8d
    shl     edx, 2
    mov     eax, [rdi + rdx]
	ret
