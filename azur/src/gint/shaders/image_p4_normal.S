.global _azrp_image_shader_p4_normal
#include "image_macros.S"

/* P4 Opaque rendering, Azur version: trivial with loop transforms.

   This is a pretty direct loop with no difficult tricks involved; it expands
   on P8 by adding another edge pointer. The main change is the decoding logic
   which now only involves a single byte to load for every two pixels, but more
   arithmetic to extract the nibbles.

   All the loops in Azur's P4 functions are obvious EX chains and thus any
   optimization would need to simplify the arithmetic to gain any half-cycles.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  Right edge value
   r9:  Palette
   r10: Left edge pointer
   r11: Left edge value
   r12: Edge stride
   r13: [temporary]
   r14: [temporary] */

.macro GEN_NORMAL_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	shlr	r2
	nop

	add	r10, r10
	nop

	mov.l	@r8+, r9	/* cmd.palette */
	mov	r2, r0

	mov.w	@r8+, r7	/* cmd.edge_2 */
	shll2	r0

	mov.l	r12, @-r15
	shll	r7

	mov.l	r11, @-r15
	add	r5, r7

	mov	r0, r12
	add	r6, r12

	mov.l	r13, @-r15
	add	r5, r10

	mov.l	r14, @-r15
	add	#-4, r5

	add	#-1, r4		/* Input stride compensation for pipelining */
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

	mov.b	@r3+, \TMP1
	mov	#-4, \TMP2

	mov.w	@r7, r8		/* Save right edge */
	nop

	mov.w	@r10, r11	/* Save left edge */
	shll	\TMP1

2:	mov	\TMP1, r0
	and	#0x1e, r0

	shld	\TMP2, \TMP1
	mov	#0x1e, \TMP2

	mov.w	@(r0,r9), r0
	and	\TMP2, \TMP1

	mov.w	r0, @(\OFF1,r5)
	mov	\TMP1, r0

	mov.b	@r3+, \TMP1
	add	#\OUT_DIR, r5

	mov.w	@(r0,r9), r0
	mov	#-4, \TMP2

	mov.w	r0, @(\OFF2,r5)
3:	shll	\TMP1

	mov.w	r8, @r7		/* Restore right edge */
	add	r12, r7

	mov.w	r11, @r10	/* Restore left edge */
	add	r12, r10

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r11
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	EPILOGUE
.endm

_azrp_image_shader_p4_normal:
	tst	#1, r0
	bf	9f

	GEN_NORMAL_LOOP 0, 4, r13, r14, 6, 0
9:	GEN_NORMAL_LOOP 1, -4, r13, r14, 0, 6
