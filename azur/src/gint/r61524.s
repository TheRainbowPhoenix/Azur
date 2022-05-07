.section .ilram, "ax"

.balign 4
.global _azrp_r61524_fragment_x1
_azrp_r61524_fragment_x1:
	mov.l	.R61524_DATA, r2
	shlr	r5

	ldrs	1f
	ldre	2f
	ldrc	r5
	nop

	/* Read a word from XRAM */
1:	mov.l	@r4+, r0
	/* Write that word to the display */
2:	mov.l	r0, @r2

	rts
	nop

.balign 4
.global _azrp_r61524_fragment_x2
_azrp_r61524_fragment_x2:
	mov.l	.R61524_DATA, r2
	nop

	/* Read a word, write it twice */
	ldrs	1f
	ldre	2f
	ldrc	r5
	nop

1:	mov.w	@r4+, r0
	nop
	mov.w	r0, @r2
	nop
	mov.w	r0, @r2
2:	nop

	sub	r5, r4
	sub	r5, r4

	/* Do that again on a second line */
	ldrs	3f
	ldre	4f
	ldrc	r5
	nop

3:	mov.w	@r4+, r0
	nop
	mov.w	r0, @r2
	nop
	mov.w	r0, @r2
4:	nop

	dt	r6
	bf	_azrp_r61524_fragment_x2

	rts
	nop

.balign 4
.R61524_DATA:
	.long	0xb4000000
