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

global Read_Aligned
global Read_1BOff
global Read_7BOff
global Read_17BOff
global Read_63BOff

section .text

;
; NOTE(casey): These ASM routines are written for the Windows
; 64-bit ABI. They expect RCX to be the first parameter (the count),
; and in the case of MOVAllBytesASM, RDX to be the second
; parameter (the data pointer). To use these on a platform
; with a different ABI, you would have to change those registers
; to match the ABI.
;

; RCX = first param (num. bytes to read)
; RDX = second param (buffer)
; R8 = third param
; R9 = volatile var 
; RAX = volatile var

; we want to read X bytes of data repeatedly so it adds up to 1GB
; each inner loop iteration reads 256 bytes
; each outer loop iteration reads X bytes
; there are N outer loop iterations -> N = 1GB / X
; there are M = X / 256 inner loop iterations

Read_Aligned:
	xor rax, rax
.loop:
	align 64
    ; Read 256 bytes
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 0x20]
    vmovdqu ymm0, [rdx + 0x40]
    vmovdqu ymm0, [rdx + 0x60]
    vmovdqu ymm0, [rdx + 0x80]
    vmovdqu ymm0, [rdx + 0xa0]
    vmovdqu ymm0, [rdx + 0xc0]
    vmovdqu ymm0, [rdx + 0xe0]
    add rax, 256
    cmp rax, rcx
    jb .loop
    ret

Read_1BOff:
	xor rax, rax
.loop:
	align 64
    ; Read 256 bytes
    vmovdqu ymm0, [rdx + 1]
    vmovdqu ymm0, [rdx + 0x21]
    vmovdqu ymm0, [rdx + 0x41]
    vmovdqu ymm0, [rdx + 0x61]
    vmovdqu ymm0, [rdx + 0x81]
    vmovdqu ymm0, [rdx + 0xa1]
    vmovdqu ymm0, [rdx + 0xc1]
    vmovdqu ymm0, [rdx + 0xe1]
    add rax, 256
    cmp rax, rcx
    jb .loop
    ret
	
Read_7BOff:
	xor rax, rax
.loop:
	align 64
    ; Read 256 bytes
    vmovdqu ymm0, [rdx + 7]
    vmovdqu ymm0, [rdx + 0x27]
    vmovdqu ymm0, [rdx + 0x47]
    vmovdqu ymm0, [rdx + 0x67]
    vmovdqu ymm0, [rdx + 0x87]
    vmovdqu ymm0, [rdx + 0xa7]
    vmovdqu ymm0, [rdx + 0xc7]
    vmovdqu ymm0, [rdx + 0xe7]
    add rax, 256
    cmp rax, rcx
    jb .loop
    ret
	
Read_17BOff:
	xor rax, rax
.loop:
	align 64
    ; Read 256 bytes
    vmovdqu ymm0, [rdx + 0x11]
    vmovdqu ymm0, [rdx + 0x31]
    vmovdqu ymm0, [rdx + 0x51]
    vmovdqu ymm0, [rdx + 0x71]
    vmovdqu ymm0, [rdx + 0x91]
    vmovdqu ymm0, [rdx + 0xB1]
    vmovdqu ymm0, [rdx + 0xD1]
    vmovdqu ymm0, [rdx + 0xF1]
    add rax, 256
    cmp rax, rcx
    jb .loop
    ret
	
Read_63BOff:
	xor rax, rax
.loop:
	align 64
    ; Read 256 bytes
    vmovdqu ymm0, [rdx + 0x3F]
    vmovdqu ymm0, [rdx + 0x5F]
    vmovdqu ymm0, [rdx + 0x7F]
    vmovdqu ymm0, [rdx + 0x9F]
    vmovdqu ymm0, [rdx + 0xBF]
    vmovdqu ymm0, [rdx + 0xDF]
    vmovdqu ymm0, [rdx + 0xFF]
    vmovdqu ymm0, [rdx + 0x11F]
    add rax, 256
    cmp rax, rcx
    jb .loop
    ret