	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_test1                          ; -- Begin function test1
	.p2align	2
_test1:                                 ; @test1
	.cfi_startproc
; %bb.0:
	mov	w8, #11520                      ; =0x2d00
	movk	w8, #305, lsl #16
	sub	x9, x2, x0
	cmp	x9, #64
	b.lo	LBB0_6
; %bb.1:
	sub	x9, x2, x1
	cmp	x9, #64
	b.lo	LBB0_6
; %bb.2:
	mov	w9, #0                          ; =0x0
	add	x10, x2, #32
	add	x11, x1, #32
	add	x12, x0, #32
LBB0_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_4 Depth 2
	mov	x13, x12
	mov	x14, x11
	mov	x15, x10
	mov	w16, #1024                      ; =0x400
LBB0_4:                                 ;   Parent Loop BB0_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x13, #-32]
	ldp	q2, q3, [x13], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	fadd.4s	v2, v2, v6
	fadd.4s	v3, v3, v7
	stp	q0, q1, [x15, #-32]
	stp	q2, q3, [x15], #64
	subs	x16, x16, #16
	b.ne	LBB0_4
; %bb.5:                                ;   in Loop: Header=BB0_3 Depth=1
	add	w9, w9, #1
	cmp	w9, w8
	b.ne	LBB0_3
	b	LBB0_10
LBB0_6:
	mov	w9, #0                          ; =0x0
LBB0_7:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_8 Depth 2
	mov	x10, #0                         ; =0x0
LBB0_8:                                 ;   Parent Loop BB0_7 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10]
	ldr	s1, [x1, x10]
	fadd	s0, s0, s1
	str	s0, [x2, x10]
	add	x10, x10, #4
	cmp	x10, #1, lsl #12                ; =4096
	b.ne	LBB0_8
; %bb.9:                                ;   in Loop: Header=BB0_7 Depth=1
	add	w9, w9, #1
	cmp	w9, w8
	b.ne	LBB0_7
LBB0_10:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
