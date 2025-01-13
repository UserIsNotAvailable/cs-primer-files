default rel
section .text
global volume
volume:
        mulss   xmm0,   xmm0
        mulss   xmm0,   [PI]
        mulss   xmm0,   xmm1
        mulss   xmm0,   [THIRD]
 	    ret
PI:     dd      0x40490fdb
THIRD:  dd      0x3eaaaaab
