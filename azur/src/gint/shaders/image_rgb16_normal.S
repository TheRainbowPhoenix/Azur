.global _azrp_image_shader_rgb16_normal
#include "image_macros.S"

/* RGB16 Opaque rendering, Azur version: by straightforward copy.

   This function of the image renderer is designed for Azur's streaming model
   only. Unlike its RAM-model counterpart which is bottlenecked by its writing
   speed, this function is entirely limited by the CPU's ability to output the
   data in the required format.

   In the simple case where there is no color effect and no HFLIP, the task of
   rendering a 16-bit opaque image boils down to a 2-dimensional memcpy. This
   task can be optimized by moving longwords if the source and destination and
   co-4-aligned, with four variations depending on the width and initial
   position, identified by the following parameters:

   * w1 / w2 denotes the parity of the command width;
   * o2 / o4 denotes the alignment of the output.

   It is easy to see that when input and output are not co-aligned, any attempt
   to combine two word reads into a single long write requires at least 3
   cycles per 2 pixels and needs parallelism over several pixels to not get
   immediately shut down by the LS-to-EX delay. Here we decide to naively copy
   by words, which achieves 4 cycles per 2 pixels, mainly because large RGB16
   images are very quickly bottlenecked in reading by their own size anyway.

   The HFLIP version also needs to rearrange pixels, and is thus performed with
   word-based copies in all situations, which is a straightforward process. */

_azrp_image_shader_rgb16_normal:
	/* Not a single cycle */
	tst	#1, r0
	bf	_BACKWARD_WORD_COPY

	mov	#8, r0		/* Use the naive method for width ≤ 8 */
	cmp/ge	r2, r0

	bt.s	_FORWARD_WORD_COPY
	nop

	mov	r5, r0		/* Check if r3 and r5 are co-aligned */
	xor	r3, r0

	/* Not a single cycle */
	tst	#2, r0
	bt	_FORWARD_LONG_COPY

_FORWARD_WORD_COPY:
	START
2:	movs.w	@r3+, x0
3:	movs.w	x0, @r5+
	END
	EPILOGUE

_FORWARD_LONG_COPY:
	shlr	r2		/* Test width parity */
	mov	#2, r0

	bt	.w1
	nop

.w2:	tst	r0, r3		/* Test alignment of input */
	bf	.w2d2

.w2d4:	START
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	END
	EPILOGUE

.w2d2:	add	#-1, r2
	nop

	START
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	END
	EPILOGUE

.w1:	tst	r0, r3		/* Test alignment of input */
	bf	.w1d2

.w1d4:	START
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	END
	EPILOGUE

.w1d2:	START
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	END
	EPILOGUE

_BACKWARD_WORD_COPY:
	mov	r2, r0
	shll	r0

	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop

	START
2:	movs.w	@r3+, x0
3:	movs.w	x0, @-r5
	END
	EPILOGUE
