.global _azrp_shader_tex2d
.align 4

/* Register assignment
   r0: (temporary)
   r1: Lines
   r2: Command queue; (temporary)
   r3: Input
   r4: [parameter] azrp_width*2; output stride
   r5: [parameter] Command queue; Output
   r6: [parameter] azrp_frag; alpha value or (temporary)
   r7: Columns
   r8: Input stride
   r9: Image profile */
_azrp_shader_tex2d:
	mov.l	r8, @-r15
	add	#2, r5

	mov.l	r9, @-r15
	mov	r5, r2

	mov.w	@r2+, r7    /* command.columns */

	mov.l	@r2+, r8    /* command.image */

	mov.w	@r2+, r5    /* command.output (offset) */
	sub	r7, r4

	mov.w	@r2+, r1    /* command.lines */
	sub	r7, r4

	mov.w	@r8+, r0    /* image.profile */
	add	r6, r5

	mov.w	@r8+, r6    /* image.alpha */

	mov.w	@r8, r8     /* image.width */

	mov.l	@r2+, r3    /* command.input (pointer) */
	mov	r0, r2

	mova	.formats, r0
	shll2	r2

	mov.l	@(r0, r2), r0
	sub	r7, r8

	jmp	@r0
	shll	r8

.align 4
.formats:
	.long	_RGB565
	.long	_RGB565A
	.long	_P8
	.long	_P4

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
	add	r4, r5;		\
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

_RGB565:
	mov	#8, r0      /* Maximum width for naive method */
	cmp/ge	r7, r0

	bt.s	_RGB565.naive
	mov	#2, r0

	/* Use naive method for opposite source/destination parity */
	mov	r5, r6
	xor	r3, r6
	tst	r0, r6
	bf	_RGB565.naive

	shlr	r7
	bt	_RGB565.wo

_RGB565.we:
	tst	r0, r5
	bf	_RGB565.we_do

_RGB565.we_de:
	TEX2D_START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	TEX2D_END()

_RGB565.we_do:
	add	#-1, r7

	TEX2D_START()
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	TEX2D_END()

_RGB565.wo:
	tst	r0, r5
	bf	_RGB565.wo_do

_RGB565.wo_de:
	TEX2D_START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	TEX2D_END()

_RGB565.wo_do:
	TEX2D_START()
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	TEX2D_END()

/* Naive method for small widths and opposite source/destination parity */
_RGB565.naive:
	TEX2D_START()
2:	movs.w	@r3+, x0
3:	movs.w	x0, @r5+
	TEX2D_END()

/* [Rendering strategy for the RGB565A format]

   Since we have to check for the alpha value in each pixel, there's really no
   longword-based optimization. Instead, we just go as fast as possible with
   each pixels, using DSP instructions because conditional execution is pretty
   damn good. This takes 4 cycles/pixel. I tried a number of reductions to
   3 cycles/pixel but could not get that to work. */

_RGB565A:
	shll16	r6
	mov	#0x0004, r0 /* DC Zero mode */

	lds	r6, y0

	lds	r0, dsr

	TEX2D_START()
2:	                        movs.w  @r3+, x0
	    pcmp    x0, y0      movx.w  @r5, x1
	dct pcopy   x1, x0
3:	                        movx.w  x0, @r5+
	TEX2D_END()

/* [Rendering strategy for the P8 format] */
_P8:
	TEX2D_START()
2:
3:	nop
	TEX2D_END()

/* [Rendering strategy for the P4 format] */
_P4:
	TEX2D_START()
2:
3:	nop
	TEX2D_END()
