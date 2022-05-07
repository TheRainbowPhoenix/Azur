.global _azrp_image_shader_p8_swapcolor
#include "image_macros.S"

/* P8 SWAPCOLOR, Azur version: by branchless xor selection.

   This is essentially the same logic as gint's P8 SWAPCOLOR version, but with
   a 2-unrolled 2-stage-pipeline since the bottleneck on RAM is now on the CPU.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  palette[cmd.color_1] ^ cmd.color_2 (ie. x ^ y)
   r9:  Palette
   r10: Holds (x ^ y) & -(c == x) during selection
   r11: cmd.color_1
   r12: Right edge stride
   r13: [temporary]
   r14: [temporary]

   Spilled to stack:
   @(-4,r15): Right edge value */

.macro GEN_SWAPCOLOR_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.l	@r8+, r9	/* cmd.palette */
	shlr	r2

	mov.w	@r8+, r7	/* cmd.edge_2 */
	mov	r2, r0

	mov.l	r12, @-r15
	shll2	r0

	mov.l	r11, @-r15
	shll	r7

	mov.w	@r8+, r11	/* cmd.color_1 */
	add	r5, r7

	mov.l	r10, @-r15
	add	#-4, r5

	mov.l	r13, @-r15
	exts.b	r11, r11

	mov	r11, r13
	add	r13, r13

	mov.w	@r8, r8		/* cmd.color_2 */
	add	r9, r13

	mov	r0, r12
	add	r6, r12

	mov.w	@r13, r13
	add	#-2, r4		/* Input stride compensation for pipelining */

	mov.l	r14, @-r15
	nop

	xor	r13, r8
	nop

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	START

	mov.b	@r3+, \TMP2
	nop

	mov.w	@r7, r0		/* Save right edge */
	nop

	mov.l	r0, @-r15
	cmp/eq	\TMP2, r11

	mov.b	@r3+, \TMP1
	add	\TMP2, \TMP2

2:	subc	r10, r10
	mov	\TMP2, r0

	cmp/eq	\TMP1, r11
	mov.w	@(r0, r9), r0

	and	r8, r10
	nop

	xor	r10, r0
	nop

	mov.w	r0, @(\OFF1, r5)
	add	#\OUT_DIR, r5

	mov.b	@r3+, \TMP2
	subc	r10, r10

	add	\TMP1, \TMP1
	mov	\TMP1, r0

	mov.w	@(r0, r9), r0
	cmp/eq	\TMP2, r11

	mov.b	@r3+, \TMP1
	and	r8, r10

	xor	r10, r0
	nop

	mov.w	r0, @(\OFF2, r5)
3:	add	\TMP2, \TMP2

	/* TODO: Use x0 as temporary storage by moving the main registers */
	mov.l	@r15+, r0
	nop

	mov.w	r0, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r10
	mov.l	@r15+, r11
	mov.l	@r15+, r12
	EPILOGUE
.endm

_azrp_image_shader_p8_swapcolor:
	tst	#1, r0
	bf	9f

	GEN_SWAPCOLOR_LOOP 0, 4, r13, r14, 4, 2
9:	GEN_SWAPCOLOR_LOOP 1, -4, r13, r14, 2, 4
