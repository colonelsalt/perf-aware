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

global Read_48K
global Read_1M
global Read_16M
global Read_32M
global Read_64M
global Read_1G

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
; R8 = volatile var
; R9 = volatile var

Read_48K:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	and r8, ~0xC000
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret

Read_1M:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	and r8, ~0x100000
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret

Read_16M:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	and r8, ~0x1000000
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret

Read_32M:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	and r8, ~0x2000000
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret
	
Read_64M:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	and r8, ~0x4000000
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret
	
Read_1G:
    xor rax, rax
	mov r8, rdx
	align 64
.loop:
    vmovdqu ymm0, [r8]
    vmovdqu ymm0, [r8 + 32]
    add rax, 64
	mov r8, rax
	add r8, rdx
    cmp rax, rcx
    jb .loop
    ret