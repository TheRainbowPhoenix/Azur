.global _azrp_shader_rect
.global _azrp_shader_rect_loop_flat
.global _azrp_shader_rect_loop_invert
.global _azrp_shader_rect_loop_darken
.global _azrp_shader_rect_loop_whiten
.align 4

#define _height r1
#define _edge_1 r2
#define _edge_2 r3
#define _stride r4
#define _cmd r5
#define _frag r6
#define _wl r7
#define _redstride r10

/* r0: (temporary)
   r1: height counter
   r2: (temporary) then fragment + edge_1
   r3: (temporary) then fragment + edge_2
   r4: stride (azrp_width * 2)
   r5: cmd then color (can also be a temporary)
   r6: fragment
   r7: longwords to write on each line (wl)
   r8: (temporary) then saved edge_1
   r9: (temporary) then saved edge_2
   r10: reduced stride (azrp_width * 2 - 4 * wl) */

_azrp_shader_rect:
	mov.w	@_cmd+, r3		/* shader_id || y */
	shll	_stride

	mov.l	r8, @-r15
	nop

	mov.b	@_cmd+, r8		/* height_total */
	extu.b	r3, r3

	mov.b	@_cmd+, _height		/* height_frag */
	mulu.w	r3, _stride

	mov.w	@_cmd+, r2		/* xl */
	mov	#-4, r0

	sub	_height, r8
	mov.b	r8, @(r0, _cmd)		/* update: height_total */

	mov.l	.azrp_frag_height, r0	/* ... inefficient ... */
	shll2	r2

	sts	macl, r3
	add	r2, _frag

	mov.l	@r0, r0
	mov	#0, r2

	mov.w	@_cmd+, _wl		/* wl */
	add	r3, _frag

	mov.l	r9, @-r15
	cmp/hs	r0, r8

	/* Next fragment height is currently r8 = remaining height
	   Set it to r0 = azrp_frag_height if r8 >= r0 */
	bf	1f
	mov	r0, r8

1:	mov	#-5, r0
	mov.b	r8, @(r0, _cmd)		/* update: height_frag */

	mov	#-7, r0
	mov.b	r2, @(r0, _cmd)		/* update: y */

	mov.w	@_cmd+, _edge_1		/* edge_1 */
	mov	_wl, r8

	mov.w	@_cmd+, _edge_2		/* edge_2 */
	shll2	r8

	mov.l	@_cmd+, r0		/* loop */
	add	_frag, _edge_1

	mov.w	@_cmd, r5		/* color */
	add	_frag, _edge_2

	mov.l	r10, @-r15
	mov	_stride, _redstride

	jmp	@r0
	sub	r8, _redstride

.azrp_frag_height:
	.long	_azrp_frag_height

.macro START
	ldrs	2f
	ldre	3f

1:	ldrc	_wl
	nop

	mov.w	@_edge_1, r8
	nop

	mov.w	@_edge_2, r9
	nop
.endm

.macro END
	dt	_height
	mov.w	r8, @_edge_1

	add	_redstride, _frag
	mov.w	r9, @_edge_2

	add	_stride, _edge_1
	nop

	bf.s	1b
	add	_stride, _edge_2

	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
.endm

_azrp_shader_rect_loop_flat:
	extu.w	r5, r0
	shll16	r5
	or	r0, r5
	lds	r5, x0

	START
	mov	_frag, r5
2:3:	movs.l	x0, @r5+
	mov	r5, _frag
	END

_azrp_shader_rect_loop_invert:
	/* Inefficient: could go all the way down to 2 cycles/long with
	   pipelining, but we're stuck at 3 cycles/long with this naive
	   approach */
	// TODO: time it we might be able to just non-pipeline and read ahead
	// also this _frag update is suspicious
	START
2:	mov.l	@_frag, r0
	not	r0, r0
	mov.l	r0, @_frag
3:	add	#4, _frag
	END
	nop

_azrp_shader_rect_loop_darken:
	mov.l	.darken_mask, r5
	nop

	/* Inefficient */
	START
2:	mov.l	@_frag, r0
	and	r5, r0
	shlr	r0
	mov.l	r0, @_frag
3:	add	#4, _frag
	nop
	END

_azrp_shader_rect_loop_whiten:
	mov.l	.darken_mask, r5
	nop

	/* Inefficient */
	START
2:	mov.l	@_frag, r0
	not	r0, r0
	and	r5, r0
	shlr	r0
	not	r0, r0
	mov.l	r0, @_frag
3:	add	#4, _frag
	nop
	END

.darken_mask:
	.long	0xf7def7de
