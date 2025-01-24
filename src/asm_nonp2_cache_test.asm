;  ========================================================================
;
;  (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
;
;  This software is provided 'as-is', without any express or implied
;  warranty. In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Please see https://computerenhance.com for more information
;
;  ========================================================================

;  ========================================================================
;  LISTING 150
;  ========================================================================

global Read_Arbitrary

section .text

;
; NOTE(casey): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (the count),
; and in the case of MOVAllBytesASM, RDX to be the second
; parameter (the data pointer). To use these on a platform
; with a different ABI, you would have to change those registers
; to match the ABI.
;

; RCX = first param (outer loop count 1GB / X)
; RDX = second param (buffer)
; R8 = third param (number of 256b chunks to read, i.e. X / 256)
; R9 = volatile var 
; RAX = buffer write pointer

; we want to read X bytes of data repeatedly so it adds up to 1GB
; each inner loop iteration reads 256 bytes
; each outer loop iteration reads X bytes
; there are N outer loop iterations -> N = 1GB / X
; there are M = X / 256 inner loop iterations

Read_Arbitrary:
.outer:
	mov rax, rdx
    mov r9, r8
	align 64
.inner:
    ; Read 256 bytes
    vmovdqu ymm0, [rax]
    vmovdqu ymm0, [rax + 0x20]
    vmovdqu ymm0, [rax + 0x40]
    vmovdqu ymm0, [rax + 0x60]
    vmovdqu ymm0, [rax + 0x80]
    vmovdqu ymm0, [rax + 0xa0]
    vmovdqu ymm0, [rax + 0xc0]
    vmovdqu ymm0, [rax + 0xe0]
    
    add rax, 256
    dec r9
	jnz .inner

    dec rcx
    jnz .outer
    ret