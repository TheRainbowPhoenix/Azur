.global _azrp_image_shader_rgb16_swapcolor
#include "image_macros.S"

/* RGB16 SWAPCOLOR, Azur version: by branchless xor selection.

   The xor selection is explained in gint's version of P8 SWAPCOLOR. This
   version's selection is slightly simpler because we don't have to index the
   palette to find the source color. We use a 2-unrolled 2-stage-pipeline loop
   to optimize for CPU speed.

   r7:  Right edge pointer
   r8:  Right edge value
   r9:  cmd.color_1
   r10: Holds (x ^ y) & -(c == x) during selection
   r11: cmd.color_1 ^ cmd.color_2 (ie. x ^ y)
   r12: Right edge stride
   r13: [temporary] */

.macro GEN_SWAPCOLOR_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.w	@r8+, r7	/* cmd.edge_2 */
	shlr	r2

	mov.l	r11, @-r15
	add	#-2, r4		/* Input stride compensation for pipelining */

	mov.w	@r8+, r9	/* cmd.color_1 */
	shll	r7

	mov.l	r10, @-r15
	add	r5, r7

	mov.l	r12, @-r15
	add	#-2, r5		/* Predecrement, see output logic */

	mov.w	@r8+, r11	/* cmd.color_2 */
	mov	r2, r12

	mov.l	r13, @-r15
	shll2	r12

	add	r6, r12
	nop

	xor	r9, r11
	nop

 .if \HFLIP
	mov	r2, r0
	shll2	r0

	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	START

	mov.w	@r3+, \TMP1
	nop

	mov.w	@r7, r8		/* Save right edge */
	nop

	cmp/eq	\TMP1, r9
	nop

2:	subc	r10, r10
	nop

	and	r11, r10
	mov.w	@r3+, \TMP2

	xor	r10, \TMP1
	nop

	mov.wv	\TMP1 \OFF1 r5
	cmp/eq	\TMP2, r9

	add	#\OUT_DIR, r5
	nop

	subc	r10, r10
	nop

	and	r11, r10
	mov.w	@r3+, \TMP1

	xor	r10, \TMP2
	nop

	cmp/eq	\TMP1, r9
3:	mov.wv	\TMP2 \OFF2 r5

	mov.w	r8, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	mov.l	@r15+, r11
	EPILOGUE
.endm

_azrp_image_shader_rgb16_swapcolor:
	tst	#1, r0
	bf	9f

	GEN_SWAPCOLOR_LOOP 0, 4, r0, r13, 2, 0
9:	GEN_SWAPCOLOR_LOOP 1, -4, r13, r0, 0, 2
