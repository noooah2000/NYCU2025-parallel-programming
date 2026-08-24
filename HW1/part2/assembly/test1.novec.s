	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_test1                          ; -- Begin function test1
	.p2align	2
_test1:                                 ; @test1
	.cfi_startproc
; %bb.0:
	mov	w8, #0                          ; =0x0
	mov	w9, #11520                      ; =0x2d00
	movk	w9, #305, lsl #16
LBB0_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_2 Depth 2
	mov	x10, #0                         ; =0x0
LBB0_2:                                 ;   Parent Loop BB0_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10]
	ldr	s1, [x1, x10]
	fadd	s0, s0, s1
	str	s0, [x2, x10]
	add	x10, x10, #4
	cmp	x10, #1, lsl #12                ; =4096
	b.ne	LBB0_2
; %bb.3:                                ;   in Loop: Header=BB0_1 Depth=1
	add	w8, w8, #1
	cmp	w8, w9
	b.ne	LBB0_1
; %bb.4:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
