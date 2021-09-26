.global _azrp_shader_tex2d
.align 4

/* Register assignment
   r0: (temporary)
   r1: Lines
   r2: Command queue; (temporary)
   r3: Input
   r4: [parameter] azrp_width*2; output stride
   r5: [parameter] Command queue; Output
   r6: [parameter] azrp_frag; alpha value; (temporary)
   r7: Columns
   r8: Image pointer; (temporary)
   r9: Input stride */
_azrp_shader_tex2d:
	mov.l	r8, @-r15
	add	#2, r5

	mov.l	r9, @-r15
	mov	r5, r2

	mov.w	@r2+, r7    /* command.columns */

	mov.l	@r2+, r8    /* command.image */

	mov.w	@r2+, r5    /* command.output (offset) */
	sub	r7, r4

	mov.w	@r8+, r9    /* image.profile */
	sub	r7, r4

	mov.w	@r2+, r1    /* command.lines */
	add	r6, r5

	mov.l	@r2+, r3    /* command.input (pointer) */
	shll2	r9

	mova	.formats, r0

	mov.w	@r8+, r6    /* image.alpha */

	mov.l	@(r0,r9), r0

	mov.w	@r8+, r9    /* image.width */

	jmp	@r0
	/* Stall for r9 */
	sub	r7, r9

.align 4
.formats:
	.long	_RGB565
	.long	_RGB565A
	.long	_NOP /* P8 */
	.long	_P4_RGB565A /* =P4 */
	.long	_P8_RGB565
	.long	_P8_RGB565A
	.long	_P4_RGB565

/* [Loop macros]

   The following macros implement the main loop of the image renderer.
   * Each line is rendered in the tight loop between 2: and 3: (both included).
   * r5 is the output (with stride r4, in bytes)
   * r3 is the input (with stride r9, in bytes)
   * There are r1 rows with r7 iterations each */

#define TEX2D_START()		\
	ldrs	2f;		\
	ldre	3f;		\
1:	ldrc	r7

#define TEX2D_END_NORET()	\
	dt	r1;		\
	add	r4, r5;		\
	bf.s	1b;		\
	add	r9, r3

#define TEX2D_END()		\
	TEX2D_END_NORET();	\
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
   start or end address is 2-aligned.

   However, when they have opposite parity, each longword read matches up with
   a 2-aligned write (or vice-versa). Rearranging words with arithmetic does
   not help because of the stall cycle between loading a register and using it
   in the ALU, which makes the minimum time 4 cycles for 2 pixels (the same as
   the word-based copy). Weaving iterations could help but would be too complex
   here (adding sub-cases); a super-heavy renderer with more hypotheses (like a
   tileset shader) should aim for that route though. Also, movua.l followed by
   mov.l is even slower (5 cycles). */
.align 4
_RGB565:
	mov	#8, r0      /* Maximum width for naive method */
	cmp/ge	r7, r0

	shll	r9

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
   each pixel, using DSP instructions because conditional execution is pretty
   damn good. This takes 4 cycles/pixel. I tried a number of reductions to
   3 cycles/pixel but could not get any of them to work. */
.align 4
_RGB565A:
	shll16	r6
	mov	#0x0004, r0 /* DC Zero mode */

	shll	r9

	lds	r6, y0

	lds	r0, dsr

	TEX2D_START()
2:	                        movs.w  @r3+, x0
	    pcmp    x0, y0      movx.w  @r5, x1
	dct pcopy   x1, x0
3:	                        movx.w  x0, @r5+
	TEX2D_END()

/* [Rendering strategy for the P8_RGB565A format]

   The work needed for each pixel gets more difficult as we go, with alpha
   being the major culprit due to its additional comparisons, jumps, and
   limited interweaving opportunities due to conditionally-executed code.

   Because arithmetic is unavoidable and there are 1-cycle delays between both
   loading-arithmetic, and arithmetic-indexing pairs, the loop has 2 interwoven
   iterations with an open structure. This fills the stall cycles and increases
   parallelism significantly. Pure interweaving handbook.

   Dealing with odd widths is a major pain as usual. Instead of adding logic to
   handle the extra pixel separately, this routine lets the loop overwrite it,
   then restores its original value afterwards - a delightfully elegant trick.

   The P8 format is actually so bad that spending precious time grinding cycles
   felt completely inappropriate without first refining it. This led to two new
   variations, P8_RGB565 and P8_RGB565A, which fix the following problems.

   -> First there is alpha for all images, which is the most costly feature,
      single-handedly accounting for half of the work per pixel. P8_RGB565
      does no support alpha, which basically doubles performance.

   -> Then, there is the alpha value itself. In P8 it is a variable (and fxconv
      sets it to 0xff), which burns a register for the comparison and enforces
      a fixed order between comparison and left-shift. P8_RGB565A always sets
      an alpha value of 0x00 which lifts both constraints.

   -> Then, there are palette indices. In P8 they are unsigned, which requires
      an extu.b. In P8_RGB565 and P8_RGB565A they are signed, so the sign-
      extended value of the mov.b can be used directly (once doubled). The
      palette base is simply offset by 128 entries, with colors numbered
      -128..-1 first and only then 0..127.

   -> Finally, there's the palette itself. In P8 it always has 256 entries,
      even when only a few are used. For small images this is a huge waste, so
      P8_RGB565 and P8_RGB565A only store colors that are actually used.

   P8_RGB565A achieves 4.5 cycles/pixel asymptotically, which is really good
   compared to 4 cycles/pixel for RGB565A. */
.align 4
_P8_RGB565A:
	mov.l	r13, @-r15
	add	#-2, r9 /* Input stride compensation for openness */

	mov	r7, r13
	shlr	r7

	mov.l	r12, @-r15
	movt	r6

	mov.l	r10, @-r15
	shll	r13

	mov.w	_P8_RGB565A.palette_distance, r0
	add	r6, r7

	sub	r6, r9

	sub	r6, r4

	sub	r6, r4

	add	r0, r8

	add	r5, r13
	mov	r7, r2

	add	#-4, r5 /* Output offset compensation in the loop */

	shll2	r2

	add	r4, r2
	nop /* 4-alignment */

	TEX2D_START()

	mov.b	@r3+, r6

	/* Save next pixel for the odd-width case */
	mov.w	@r13, r12

	mov.b	@r3+, r10
	tst	r6, r6

	/* 2-interwoven open main loop */
2:	add	r6, r6
	mov	r6, r0

	add	r10, r10
	bt.s	5f

	tst	r10, r10
	mov.w	@(r0,r8), r0

	mov.w	r0, @(4,r5)

     5: mov.b	@r3+, r6
	mov	r10, r0

	bt.s	6f
	add	#4, r5

	mov.w	@(r0,r8), r0

	mov.w	r0, @(2,r5)

     6:	mov.b	@r3+, r10
3:	tst	r6, r6

	/* Restore last pixel */
	mov.w	r12, @r13
	add	r2, r13

	TEX2D_END_NORET()
	mov.l	@r15+, r10
	mov.l	@r15+, r12
	mov.l	@r15+, r13
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

_P8_RGB565A.palette_distance:
	/* Distance between image pointer and palette array base */
	.word	260

/* [Rendering strategy for the P8_RGB565 format]

   See P8_RGB565A for format details. Removing the checks for transparency and
   the jumps simplifies the instruction sequence and allows superior
   parallelism because all paths are unconditional. This routines achieves
   3 cycles/pixel asymptotically. */
.align 4
_P8_RGB565:
	mov.l	r13, @-r15
	add	#-2, r9 /* Input stride compensation for openness */

	mov	r7, r13
	shlr	r7

	mov.l	r12, @-r15
	movt	r6

	mov.l	r10, @-r15
	shll	r13

	mov.w	_P8_RGB565.palette_distance, r0
	add	r6, r7

	sub	r6, r9

	sub	r6, r4

	sub	r6, r4

	add	r0, r8

	add	r5, r13

	add	#-4, r5 /* Output offset compensation in the loop */
	mov	r7, r2

	shll2	r2

	add	r4, r2
	nop /* 4-alignment */

	TEX2D_START()

	mov.b	@r3+, r0

	/* Save next pixel for the odd-width case */
	mov.w	@r13, r12

	mov.b	@r3+, r10
	shll	r0

	/* 2-interwoven open main loop */
2:	mov.b	@r3+, r6
	shll	r10

	mov.w	@(r0,r8), r0

	mov.w	r0, @(4,r5)
	mov	r10, r0

	mov.b	@r3+, r10
	add	#4, r5

	mov.w	@(r0,r8), r0
	shll	r6

	mov.w	r0, @(2,r5)
3:	mov	r6, r0

	/* Restore last pixel */
	mov.w	r12, @r13
	add	r2, r13

	TEX2D_END_NORET()
	mov.l	@r15+, r10
	mov.l	@r15+, r12
	mov.l	@r15+, r13
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

_P8_RGB565.palette_distance:
	/* Distance between image pointer and palette array base */
	.word	260

/* [Rendering strategy for the P4_RGB565A format]

   This is the most complex format. Most of the remarks that apply to
   P8_RGB565A also apply here, except that there are less opportunities to save
   computation because nibbles must be extracted anyway.

   The P4_RGB565A format is simply bopti's P4, but an additional variation
   P4_RGB565 is specified to save on transparency handling, which is very
   expensive.

   The special nature of the nibble packing means the simplest loop form writes
   2 pixels from a 2-aligned source image position in a single iteration. Other
   structures don't even come close: selecting nibbles individually is folly,
   while not interweaving is inefficient. So the whole point of this routine is
   to forcibly align the subimage on a byte-aligned and never break that grid.

   The command builder for P4 does this alignment before submitting the
   command. Obviously the transform can cause one extra pixel to be overridden
   on each side of every line. The command is thus extended with two edge
   offsets indicating pixels to preserve at each end. When overwrites occurs,
   the edge offsets point to the overwritten pixels so they can be restored.
   Otherwise, they point to the next pixels and the restores are no-ops. See
   the strategy used for managing interweaving in P8 formats for details.

   TODO: Asymptotic performance */
.align 4
_P4_RGB565A:
	mov.l	r10, @-r15
	shlr	r9

	mov.l	r11, @-r15
	add	#-1, r9 /* Input stride compensation for openness */

	mov.l	r12, @-r15
	add	#2, r8 /* image.palette */

	mov.w	@r2+, r11 /* command.edge1 */
	shlr	r7

	mov.w	@r2+, r12 /* command.edge2 */
	mov	r5, r10

	mov.l	r13, @-r15
	shll	r11

	mov.l	r14, @-r15
	shll	r12

	TEX2D_START()

	mov	r10, r0
	mov.b	@r3+, r6

	/* Stall for r0 */

	mov.w	@(r0,r11), r13

	mov.w	@(r0,r12), r14

	/* Main loop with 2 pixels sharing a single byte */

2:	/* Stall for r6 */

	shll	r6

	mov	r6, r0
	and	#0x1e, r0

	tst	r0, r0

	bt	4f
	mov.w	@(r0,r8), r0

	mov.w	r0, @(2,r5)
     4:	shlr2	r6

	shlr2	r6

	mov	r6, r0
	and	#0x1e, r0

	tst	r0, r0

	bt	5f
	mov.w	@(r0,r8), r0

	mov.w	r0, @r5

     5: mov.b	@r3+, r6
3:	add	#4, r5

	mov	r10, r0
	add	r7, r10

	/* Stall for r0 */

	mov.w	r13, @(r0,r11)
	add	r7, r10

	mov.w	r14, @(r0,r12)
	add	r4, r10

	add	r7, r10
	add	r7, r10

	TEX2D_END_NORET()
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

/* [Rendering strategy for the P4_RGB565 format]
   Same as P4_RGB565A without transparency checks (fairly straightforward). */
.align 4
_P4_RGB565:
	TEX2D_START()
2:
3:	nop
	TEX2D_END()

/* [Unsupported formats]
   P8 is unsupported, use P8_RGB565 and P8_RGB565A. */
_NOP:
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
