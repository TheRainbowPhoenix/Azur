.global _azrp_image_shader_rgb16_clearbg
#include "image_macros.S"

/* RGB16 CLEARBG and DYE, Azur version: by NULL canceling.

   This function handles both CLEARBG and DYE, which happen to work identically
   on RGB16, save for the fact that the DYE loop ignores the value of opaque
   pixels and uses the dye color instead. It's one of the standard 2-unrolled
   2-stage-pipeline loops with a right edge, using NULL canceling for
   transparency.

   r0:  [temporary] (CLEARBG) or dye value (DYE)
   r7:  Right edge pointer
   r8:  Right edge value
   r9:  Background color
   r10: Nullable output pointer
   r11: 0 (to neutralize addc during NULL-cancelling)
   r12: Right edge stride
   r13: [temporary] (one of the pixels)
   r14: [temporary] (one of the pixels in DYE)

   The GEN_CLEARBG_LOOP macro parameters are as follows. All of them except for
   SRC1 and SRC2 are determined by HFLIP; it's just simpler to set their values
   on the macro's call site than have .if statements everywhere. This set of
   parameters is used for virtually all the functions of all the formats.

   SRC1 and SRC2 are used in DYE mode to replace the pixel values read from
   memory with a constant register.

   HFLIP: Whether to enable HFLIP
   OUT_DIR: Variation of r5 at each loop, either 4 or -4
   TMP1: Temporary register for first pixel
   TMP2: Temporary register for second pixel
   OFF1: Offset for first pixel write
   OFF2: Offset for second pixel write
   SRC1: Source of first write (here either TMP1 or r0)
   SRC2: Source of second write (here either TMP2 or r0) */

.macro GEN_CLEARBG_DYE_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2, SRC1, SRC2
	mov.w	@r8+, r7	/* cmd.edge_2 */
	shlr	r2

	mov.l	r11, @-r15
	mov	#0, r11

	mov.w	@r8+, r9	/* cmd.color_1 */
	shll	r7

	mov.l	r10, @-r15
	add	r5, r7

	mov.l	r12, @-r15
	add	#-2, r5		/* Pre-decrement, see output logic */

	mov	r2, r12
	shll2	r12

	mov.l	r13, @-r15
	add	r6, r12

	mov.l	r14, @-r15
	add	#-2, r4		/* Input stride compensation for pipelining */

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

	mov.w	@r8+, r0	/* cmd.color_2 */
	nop

	START

	mov.w	@r3+, \TMP1
	nop

	mov.w	@r7, r8		/* Save right edge */
	nop

	cmp/eq	\TMP1, r9
	nop

2:	mov	#-1, r10
	addc	r11, r10

	mov.w	@r3+, \TMP2
	and	r5, r10

	add	#\OUT_DIR, r5
	nop

	mov.wv	\SRC1, \OFF1, r10
	cmp/eq	\TMP2, r9

	mov	#-1, r10
	addc	r11, r10

	mov.w	@r3+, \TMP1
	and	r5, r10

	cmp/eq	\TMP1, r9
3:	mov.wv	\SRC2, \OFF2, r10

	mov.w	r8, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	mov.l	@r15+, r11
	EPILOGUE
.endm

#ifndef AZRP_RGB16_DYE

_azrp_image_shader_rgb16_clearbg:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_DYE_LOOP 0, 4, r0, r13, 2, 0, r0, r13
9:	GEN_CLEARBG_DYE_LOOP 1, -4, r13, r0, 0, 2, r13, r0

#endif
