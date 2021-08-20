.global _azrp_shader_clear
.align 4

_azrp_shader_clear:
	ldrs	1f
	ldre	2f
	ldrc	r4

	mov.w	@(2,r5), r0
	shll2	r4

	add	r4, r6

	swap.w	r0, r1

	xtrct	r0, r1

1: 2:	mov.l	r1, @-r6

	rts
	nop
