/* Azur's built-in shaders: <image>

   If there ever was a fantastic piece of assembler engineering in my work up
   to this point, this would be it. Every trick in the book is used here, from
   clever instruction combinations, pipeline flow and tricky DSP abuse all the
   way up to memory layout planning, transforms on loop structures, and most
   critically superscalar parallelism.

   While the performance of the shader is not *strictly* proportional to the
   speed of the tightest loop, it's very close. The use of operand-bus XRAM for
   graphics data, systematic alignment, and detailed pipeline stalling
   measurements for common instruction sequences in gintctl allow very accurate
   speed predictions to be made based on the tightness of the code.

   The palette formats of bopti have been refined for the purpose of this
   shader, with P8 being split into P8_RGB565A and P8_RGB565 with big changes,
   and P4 being renamed P4_RGB565A with minimal changes along with a variation
   aptly named P4_RGB565.

   The asymptotic performance for each format is as follows:
   * RGB565:      1    cycle/pixel if source and destination align
                  2   cycles/pixel otherwise
   * RGB565A:     4   cycles/pixel
   * P8_RGB565A:  4.5 cycles/pixel
   * P8_RGB565:   3   cycles/pixel
   * P4_RGB565A:  5   cycles/pixel
   * P4_RGB565:   3.5 cycles/pixel

   Entirely documenting this code would take me hours, but some elements are
   provided in the comments. Superscalar parallelism is most easily appreciated
   by reading the two-page section 4.2 of the SH4AL-DSP manual. The other main
   structural technique at play in this code is loop transforms.

   Basically, a loop that loads a pixel, performs computations with it, and
   writes the result is inefficient because of the RAW dependencies on most
   operations (with full stall cycles between loads and computations, and
   between computations and uses as addresses). Well-established loop
   optimization literature has lots of techniques to help with this problem,
   and I use two here:

   * _Pipelining_ the loop consists in handling a single pixel over several
     iterations by doing a little bit of work in each iteration. The data for
     the pixel would move from register to register at each iteration, with the
     loop code doing one stage's worth of computation on each register. (You
     can view it as a diagonal iteration pattern in the pixel*instruction grid
     if you like such visualizations.)

     By increasing the number of pixels in the pipeline, a lot of independent
     data can be obtained, reducing dependency pressure and allowing for
     greater parallelism at the cost of more registers being used.

     The use of pipelining in this shader is very modest, with 2 stages at
     most, and usually only a couple of instructions being performed in advance
     for the next pixel while the current one finishes processing. Register
     assignments have some subtleties though since pressure is high overall.

   * _Unrolling_ iterations of the loop consists in loading two (or more)
     pixels at the start of each iteration so that we can work on one while
     waiting for stalls and dependencies on the other.

     Unlike pipelining, a loop iteration starts and ends with full pixels and
     no work carries between iterations. Unrolling allows different pixels to
     use different registers and generally better optimize the instruction
     sequence, at the cost of only supporting pixel counts that are multipes of
     the unrolling level.

     Handling non-multiple sizes is the everlasting bane of unrolled loops,
     sometimes requiring duplicate code. Smart maneuvers are used in P8 and P4
     to only handle even sizes and neutralize unwanted pixels after the fact.

   Both techniques are used simultaneously, with 2-unrolled 2-stage loops for
   almost all formats (except RGB556A which performs DSP trickery).
*/

.global _azrp_shader_image
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
_azrp_shader_image:
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
	nop

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

#define START()			\
	nop; /* 4-alignment */	\
	ldrs	2f;		\
	ldre	3f;		\
1:	ldrc	r7

#define END_NORET()		\
	dt	r1;		\
	add	r4, r5;		\
	bf.s	1b;		\
	add	r9, r3

#define END()			\
	END_NORET();		\
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
   the word-based copy). Unrolling iterations could help but would be too
   complex here (adding sub-cases); a super-heavy renderer with more hypotheses
   (like a tileset shader) should aim for that route though. Also, movua.l
   followed by mov.l is even slower (5 cycles). */
.align 4
_RGB565:
	mov	#8, r0      /* Maximum width for naive method */
	sub	r7, r9

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

/* This is 4-aligned */
_RGB565.we_de:
	START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	END()

.align 4
_RGB565.we_do:
	add	#-1, r7

	START()
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	END()

.align 4
_RGB565.wo:
	tst	r0, r5
	bf	_RGB565.wo_do

_RGB565.wo_de:
	START()
2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+

	movs.w	@r3+, x0
	movs.w	x0, @r5+
	END()

.align 4
_RGB565.wo_do:
	START()
	movs.w	@r3+, x0
	movs.w	x0, @r5+

2:	movs.l	@r3+, x0
3:	movs.l	x0, @r5+
	END()

/* Naive method for small widths and opposite source/destination parity */
.align 4
_RGB565.naive:
	START()
2:	movs.w	@r3+, x0
3:	movs.w	x0, @r5+
	END()

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

	sub	r7, r9

	shll	r9

	lds	r6, y0

	lds	r0, dsr

	START()
2:	                        movs.w  @r3+, x0
	    pcmp    x0, y0      movx.w  @r5, x1
	dct pcopy   x1, x0
3:	                        movx.w  x0, @r5+
	END()

/* [Rendering strategy for the P8_RGB565A format]

   The work needed for each pixel gets more difficult as we go, with alpha
   being the major culprit due to its additional comparisons, jumps, and
   limited optimization opportunities when unrolling due to conditionally-
   executed code.

   Because arithmetic is unavoidable and there are 1-cycle delays between both
   loading-arithmetic, and arithmetic-indexing pairs, the loop has 2-unrolled
   iterations with a 2-stage pipeline structure. This fills the stall cycles
   and increases parallelism significantly. Pure loop optimization handbook.

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
	sub	r7, r9

	mov	r7, r13
	add	#-2, r9 /* Input stride compensation for pipelining */

	mov.l	r12, @-r15
	shlr	r7

	mov.l	r10, @-r15
	movt	r6

	mov.w	_P8_RGB565A.palette_distance, r0
	shll	r13

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

	START()

	mov.b	@r3+, r6

	/* Save next pixel for the odd-width case */
	mov.w	@r13, r12

	mov.b	@r3+, r10
	tst	r6, r6

	/* 2-unrolled 2-stage main loop */
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

	END_NORET()
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
	sub	r7, r9

	mov	r7, r13
	add	#-2, r9 /* Input stride compensation for pipelining */

	mov.l	r12, @-r15
	shlr	r7

	mov.l	r10, @-r15
	movt	r6

	mov.w	_P8_RGB565.palette_distance, r0
	shll	r13

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

	START()

	mov.b	@r3+, r0

	/* Save next pixel for the odd-width case */
	mov.w	@r13, r12

	mov.b	@r3+, r10
	shll	r0

	/* 2-unrolled 2-stage main loop */
2:	mov.b	@r3+, r6
	shll	r10

	mov.w	@(r0,r8), r0
	/* This nop is not for show, it actually prevents the loop from slowing
	   down to 7 cycles /i, probably due to instruction reads alignment. */
	nop

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

	END_NORET()
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
   while not unrolling is inefficient. So the whole point of this routine is to
   forcibly align the subimage on a byte-aligned and never break that grid.

   The command builder for P4 does this alignment before submitting the
   command. Obviously the transform can cause one extra pixel to be overridden
   on each side of every line. The command is thus extended with two edge
   offsets indicating pixels to preserve at each end. When overwrites occurs,
   the edge offsets point to the overwritten pixels so they can be restored.
   Otherwise, they point to the next pixels and the restores are no-ops. See
   the strategy used for managing unrolling in P8 formats for details.

   The only irregularity is image width, which the command builder cannot
   modify. It is rounded up to the next multiple of 2, then halved. There is a
   nice trick for this operation, which is [shlr rX] then adding T to rX. We
   also need to add -1 for another adjustement, and both are combined into an
   addc, which saves one add and one movt off the EX critical chain.

   The main loop achieves 5 cycles/pixel. */
.align 4
_P4_RGB565A:
	shlr	r9
	mov	#-1, r0

	mov.l	r10, @-r15
	addc	r0, r9

	mov.l	r11, @-r15
	shlr	r7

	mov.l	r12, @-r15
	sub	r7, r9

	mov.w	@r2+, r11	/* command.edge1 */
	add	#2, r8		/* image.palette */

	mov.w	@r2+, r12	/* command.edge2 */
	mov	r5, r0

	mov.l	r13, @-r15
	shll	r11

	mov.l	r14, @-r15
	shll	r12

	add	#-4, r5
	nop	/* 4-alignment */

	START()

	mov.b	@r3+, r6
	mov	r0, r10

	mov.w	@(r0,r11), r13

	mov.w	@(r0,r12), r14
	shll	r6

	/* Main loop with 2 pixels sharing a single byte */
2:	mov	r6, r0
	and	#0x1e, r0

	tst	r0, r0

	bt.s	4f
	shlr2	r6

	mov.w	@(r0,r8), r0

	mov.w	r0, @(6,r5)
     4:	shlr2	r6

	mov	r6, r0
	and	#0x1e, r0

	tst	r0, r0
	mov.b	@r3+, r6

	bt.s	5f
	add	#4, r5

	mov.w	@(r0,r8), r0

	mov.w	r0, @r5
3:   5: shll	r6

	mov	r10, r0
	mov	r7, r10

	shll2	r10

	mov.w	r13, @(r0,r11)
	add	r4, r10

	mov.w	r14, @(r0,r12)
	add	r0, r10

	mov	r10, r0
	/* Parallelizes with [dt r1] expanded from END_NORET() */

	END_NORET()
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

/* [Rendering strategy for the P4_RGB565 format]
   Same as P4_RGB565A without transparency checks (fairly straightforward). The
   core loop runs in 3.5 cycles/pixel. */
.align 4
_P4_RGB565:
	shlr	r9
	mov	#-1, r0

	mov.l	r10, @-r15
	addc	r0, r9

	mov.l	r11, @-r15
	shlr	r7

	mov.l	r12, @-r15
	sub	r7, r9

	mov.w	@r2+, r11	/* command.edge1 */
	add	#2, r8		/* image.palette */

	mov.w	@r2+, r12	/* command.edge2 */
	mov	r5, r0

	mov.l	r13, @-r15
	shll	r11

	mov.l	r14, @-r15
	shll	r12

	add	#-4, r5
	mov	#0x1e, r2

	START()

	mov.b	@r3+, r6
	mov	#-4, r10

	mov.l	r0, @-r15

	mov.w	@(r0,r11), r13

	mov.w	@(r0,r12), r14
	shll	r6

	/* Main loop with 2 pixels sharing a single byte */
2:	mov	r6, r0
	and	#0x1e, r0

	shld	r10, r6

	mov.w	@(r0,r8), r0
	and	r2, r6

	mov.w	r0, @(6,r5)
	mov	r6, r0

	mov.b	@r3+, r6
	add	#4, r5

	mov.w	@(r0,r8), r0

	mov.w	r0, @r5
3:	shll	r6

	mov.l	@r15+, r0
	mov	r7, r10

	shll2	r10

	mov.w	r13, @(r0,r11)
	add	r4, r10

	mov.w	r14, @(r0,r12)
	add	r0, r10

	mov	r10, r0
	/* Parallelizes with [dt r1] expanded from END_NORET() */

	END_NORET()
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

/* [Unsupported formats]
   P8 is unsupported, use P8_RGB565 and P8_RGB565A. */
_NOP:
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
