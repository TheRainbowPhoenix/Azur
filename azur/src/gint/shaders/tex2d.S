.global _azrp_shader_tex2d
.align 4

/* Register assignment
   r1: Lines
   r2: Columns
   r3: Input
   r4: Output
   r5: Command queue
   r7: Constant 396*2 = 0x318
   r8: Output stride
   r9: Input stride */
_azrp_shader_tex2d:
	mov.l	r8, @-r15
	add	#2, r5

	mov.l	r9, @-r15
	mov	#0x03, r7

	ldrs	1f
	shll8	r7

	ldre	2f
	add	#0x18, r7

	/* CHECK: 4-alignment here */

.texture:
	mov.w	@r5+, r2 /* Columns */
	mov	r7, r8

	mov.l	@r5+, r3 /* Input */

	mov	r2, r0
	mov.l	@r5+, r4 /* Output */

	shll	r0
	mov.w	@r5+, r1 /* Lines */

	sub	r0, r8
	mov.w	@r5+, r9 /* Input stride */

	shlr	r2

.line:
	ldrc	r2
	dt	r1

1:	movs.l	@r3+, x0
2:	movs.l	x0, @r4+

	add	r8, r4

	bf.s	.line
	add	r9, r3

.end:
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
