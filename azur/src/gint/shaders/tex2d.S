.global _azrp_shader_tex2d
.align 4

/* Profile values from bopti */
#define PX_RGB565   0
#define PX_RGB565A  1
#define PX_P8       2
#define PX_P4       3

/* Register assignment
   r0: (temporary)
   r1: Lines
   r2: Output
   r3: Input
   r4: [parameter] azrp_width*2; output stride
   r5: [parameter] Command queue; (temporary)
   r6: [parameter] azrp_frag; (temporary)
   r7: Columns
   r8: Input stride
   r9: Image profile */
_azrp_shader_tex2d:
	mov.l	r8, @-r15
	add	#2, r5

	mov.l	r9, @-r15

	mov.w	@r5+, r7    /* command.columns */

	mov.l	@r5+, r8    /* command.image */

	mov.w	@r5+, r2    /* command.output (offset) */
	sub	r7, r4

	mov.w	@r5+, r1    /* command.lines */
	sub	r7, r4

	mov.w	@r8+, r0    /* image.profile */
	add	r6, r2

	mov.w	@r8+, r6    /* image.alpha */
	cmp/eq	#PX_P4, r0

	mov.w	@r8, r8     /* image.width */

	mov.l	@r5+, r3    /* command.input (pointer) */

	sub	r7, r8

	bt.s	.format_P4
	shll	r8

	cmp/eq	#PX_P8, r0

	bt	.format_P8
	cmp/eq	#PX_RGB565A, r0

	bt	.format_RGB565A

	/* Default below is .format_RGB565 */

/* [Loop macros]

   The following macros implement the main loop of the image renderer.
   * Each line is rendered in the tight loop between 2: and 3: (both included).
   * r2 is the output (with stride r4, in bytes)
   * r3 is the input (with stride r8, in bytes)
   * There are r1 rows with r7 iterations each */

#define TEX2D_START()		\
	ldrs	2f;		\
	ldre	3f;		\
				\
1:	ldrc	r7;		\
	dt	r1;		\

#define TEX2D_END()		\
	add	r4, r2;		\
	bf.s	1b;		\
	add	r8, r3;		\
				\
	mov.l	@r15+, r9;	\
	rts;			\
	mov.l	@r15+, r8

/* [Rendering strategy for the RGB565 format]

   In RGB565, all pixels are copied verbatim. This is a 2D memcpy, which we can
   optimize by moving longwords. Since longwords are pairs of pixels, there are
   variations and subcases based on the parity of each parameter:

   * w[eo] denotes whether the width of the image is even or odd;
   * d[eo] denotes whether the memory accesses to the source and destination
     are even (4-aligned) or odd (2-aligned).

   When the destination and source have identical parity, the d[eo] variation
   can be defined. In this case the copy is pretty direct, it's a longword copy
   and it takes 2 cycles to copy 4 bytes, plus some extra at the edges if the
   start or end address if 2-aligned.

   However, when they have opposite parity, each longword read matches up with
   a 2-aligned write (or vice-versa). Rearranging words with arithmetic does
   not help because of the stall cycle between loading a register and using it
   in the ALU, which makes the minimum time 4 cycles for 2 pixels (the same as
   the word-based copy). Weaving iterations could help but would be too complex
   here (adding sub-cases); a super-heavy renderer with more hypotheses (like a
   tileset shader) should aim for that route though. Also, movua.l followed by
   mov.l is even slower (5 cycles). */

.format_RGB565:
	mov	#8, r0      /* Maximum width for naive method */
	cmp/ge	r7, r0

	bt.s	.naive
	mov	#2, r0

	/* Use naive method for opposite source/destination parity */
	mov	r2, r6
	xor	r3, r6
	tst	r0, r6
	bf	.naive

	shlr	r7
	bt	.wo

.we:
	tst	r0, r2
	bf	.we_do

.we_de:
	TEX2D_START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r2+
	TEX2D_END()

.we_do:
	add	#-1, r7

	TEX2D_START()
	movs.w	@r3+, x0
	movs.w	x0, @r2+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r2+

	movs.w	@r3+, x0
	movs.w	x0, @r2+
	TEX2D_END()

.wo:
	tst	r0, r2
	bf	.wo_do

.wo_de:
	TEX2D_START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r2+

	movs.w	@r3+, x0
	movs.w	x0, @r2+
	TEX2D_END()

.wo_do:
	TEX2D_START()
	movs.w	@r3+, x0
	movs.w	x0, @r2+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r2+
	TEX2D_END()

/* Naive method for small widths and opposite source/destination parity */
.naive:
	TEX2D_START()
2:	movs.w	@r3+, x0
3:	movs.w	x0, @r2+
	TEX2D_END()

/* [Rendering strategy for the RGB565A format]

   Since we have to check for the alpha value in each pixel, there's really no
   longword-based optimization. Instead, we just go as fast as possible with
   each pixels, using DSP instructions. Branchless jump is pretty useful.

   TODO: Opening iterations will definitely save at least 1 cycle per pixel; it
         just requires a subcase for extremely small images (width = 1). */

.format_RGB565A:
	mov	r2, r5

	TEX2D_START()
	/* In the comparison, DC=1 if x0 == image.alpha */
2:	                         movs.w  @r3+, x0
	     pcmp    x0, y0      movx.w  @r5, x1
	dct  pcopy   x1, x0
3:	     movx.w  x0, @r5+
	TEX2D_END()

/* [Rendering strategy for the P8 format] */
.format_P8:
	TEX2D_START()
2:
3:
	TEX2D_END()

/* [Rendering strategy for the P4 format] */
.format_P4:
	TEX2D_START()
2:
3:
	TEX2D_END()
