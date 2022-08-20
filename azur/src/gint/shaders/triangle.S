.global _azrp_shader_triangle
.align 4

#define _frag r6
#define _color r7
#define _u r8
#define _v r9
#define _w r10
#define _du_x r11
#define _dv_x r12
#define _dw_x r13

/* r0: (temporary)
   r1: y counter
   r2: (temporary)
   r3: x_max - x_min + 1
   r4: azrp_width * 2
   r5: cmd
   r6: frag
   r7: color
   r8-r10: u-w
   r11-r13: d*_x
   r14: #-31 (for NULL canceling) */
_azrp_shader_triangle:
	mov.w	@r5+, r0	/* y */
	shll	r4

	mov.w	@r5+, r1	/* height_frag (y counter) */
	nop

	mov.w	@r5+, r2	/* x_min */
	extu.b	r0, r0

	mov.w	@r5+, r3	/* x_max */
	extu.b	r1, r1

	mov.w	@r5+, _color	/* color */
	mulu.w	r4, r0

	add	#2, r5
	nop

	mov.l	r8, @-r15
	sub	r2, r3

	mov.l	r9, @-r15
	add	#1, r3

	mov.l	r10, @-r15
	sts	macl, r0

	mov.l	r11, @-r15
	add	r0, _frag

	ldrs	2f
	add	r2, _frag

	ldre	3f
	add	r2, _frag

	mov.l	r12, @-r15
	sub	r3, r4

	mov.l	r13, @-r15
	sub	r3, r4

	mov.l	r14, @-r15
	mov	#-31, r14

	mov.l	@r5, _u
	mov.l	@(4, r5), _v
	mov.l	@(8, r5), _w
	mov.l	@(12, r5), _du_x
	mov.l	@(16, r5), _dv_x
	mov.l	@(20, r5), _dw_x

	/* Loop on y */
1:	ldrc	r3
	nop

	/* Loop on x */
2:	mov	_u, r0
	or	_v, r0

	or	_w, r0
	nop

	shad	r14, r0
	nop

	not	r0, r0
	nop

	and	_frag, r0
	nop

	add	#2, _frag
	nop

	add	_du_x, _u
	mov.w	_color, @r0

	add	_dv_x, _v
	nop

	add	_dw_x, _w
3:	nop

	/* Sloooow */
	add	r4, _frag
	mov.l	@(24, r5), r0

	dt	r1
	mov.l	@(28, r5), r2

	add	r0, _u
	mov.l	@(32, r5), r0

	add	r2, _v
	nop

	bf.s	1b
	add	r0, _w

	add	#-12, r5
	mov	#0, r0

	/* 1 cycle lost */

	mov.l	_u, @(12, r5)
	mov.l	_v, @(16, r5)
	mov.l	_w, @(20, r5)
	mov.b	r0, @(1, r5)

	/* Slooow */
	mov.b	@(3, r5), r0
	extu.b	r0, r0
	mov	r0, r2
	mov.b	@(2, r5), r0
	extu.b	r0, r0
	sub	r2, r0
	mov.b	r0, @(2, r5)

	mov.l	.azrp_frag_height, r1
	mov.l	@r1, r1

	cmp/ge	r0, r1
	bt	4f
	mov	r1, r0

4:	mov.b	r0, @(3, r5)

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

.balign 4
.azrp_frag_height:
	.long	_azrp_frag_height
