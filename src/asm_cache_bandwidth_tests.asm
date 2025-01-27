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

global Read_256

section .text

;
; NOTE(casey): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (the count),
; and in the case of MOVAllBytesASM, RDX to be the second
; parameter (the data pointer). To use these on a platform
; with a different ABI, you would have to change those registers
; to match the ABI.
;

; RCX = first param
; RDX = second param
; R8 = third param
; R9 = volatile var

Read_256:
    xor r9, r9
    mov rax, rdx
	align 64

.loop:
    ; Read 256 bytes
    vmovdqu ymm0, [rax]
    vmovdqu ymm0, [rax + 0x20]
    vmovdqu ymm0, [rax + 0x40]
    vmovdqu ymm0, [rax + 0x60]
    vmovdqu ymm0, [rax + 0x80]
    vmovdqu ymm0, [rax + 0xa0]
    vmovdqu ymm0, [rax + 0xc0]
    vmovdqu ymm0, [rax + 0xe0]
    
    ; Advance and mask the read offset
    add r9, 0x100
    and r9, r8

    ; Update the read base pointer to point to the new offset
    mov rax, rdx
    add rax, r9
    
    ; Repeat
    sub rcx, 0x100
    jnz .loop
    
    ret