.global _azrp_image_shader_p8_normal
#include "image_macros.S"

/* P8 Opaque rendering, Azur version: trivial with loop transforms.

   This is fairly straightforward, with no particular tricks; just index the
   palette as fast as possible in a 2-unrolled 2-stage-pipeline loop that maxes
   out CPU speed.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  Right edge value
   r9:  Palette
   r10: [temporary]
   r11: [temporary]
   r12: Right edge stride */

.macro GEN_NORMAL_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.l	@r8+, r9	/* cmd.palette */
	shlr	r2

	mov.w	@r8+, r7	/* cmd.edge_2 */
	mov	r2, r0

	mov.l	r12, @-r15
	shll2	r0

	mov.l	r10, @-r15
	shll	r7

	mov.l	r11, @-r15
	add	r5, r7

	mov	r0, r12
	add	r6, r12

	add	#-4, r5
	nop

	add	#-2, r4		/* Input stride compensation for pipelining */
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

	mov.b	@r3+, r0
	nop

	mov.w	@r7, r8		/* Save right edge */
	nop

	mov.b	@r3+, \TMP1
	shll	r0

2:	mov.b	@r3+, \TMP2
	shll	\TMP1

	mov.w	@(r0,r9), r0
	/* Fun fact: omitting this nop slows the loop to 7 cycles/i */
	nop

	mov.w	r0, @(\OFF1,r5)
	mov	\TMP1, r0

	mov.b	@r3+, \TMP1
	add	#\OUT_DIR, r5

	mov.w	@(r0,r9), r0
	shll	\TMP2

	mov.w	r0, @(\OFF2,r5)
3:	mov	\TMP2, r0

	mov.w	r8, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r12
	EPILOGUE
.endm

_azrp_image_shader_p8_normal:
	tst	#1, r0
	bf	9f

	GEN_NORMAL_LOOP 0, 4, r10, r11, 4, 2
9:	GEN_NORMAL_LOOP 1, -4, r10, r11, 2, 4
