	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_test1                          ; -- Begin function test1
	.p2align	2
_test1:                                 ; @test1
	.cfi_startproc
; %bb.0:
	mov	w8, #0                          ; =0x0
	add	x9, x2, #32
	add	x10, x1, #32
	add	x11, x0, #32
	mov	w12, #11520                     ; =0x2d00
	movk	w12, #305, lsl #16
LBB0_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_2 Depth 2
	mov	x13, x11
	mov	x14, x10
	mov	x15, x9
	mov	w16, #1024                      ; =0x400
LBB0_2:                                 ;   Parent Loop BB0_1 Depth=1
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
	b.ne	LBB0_2
; %bb.3:                                ;   in Loop: Header=BB0_1 Depth=1
	add	w8, w8, #1
	cmp	w8, w12
	b.ne	LBB0_1
; %bb.4:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
