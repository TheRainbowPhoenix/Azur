.global _azrp_shader_tex2d
.align 4

/* Register assignment
   r0: (temporary)
   r1: Lines
   r2: Output
   r3: Input
   r4: Output stride (initially uniform: azrp_width*2)
   r5: Command queue; (temporary)
   r6: (temporary) (initially azrp_frag)
   r7: Columns
   r8: Input stride */
_azrp_shader_tex2d:
	add	#2, r5

	mov.w	@r5+, r7    /* Columns */

	mov.l	r8, @-r15

	mov.w	@r5+, r0    /* Input (1/2) */
	sub	r7, r4

	mov.w	@r5+, r3    /* Input (2/2) */
	sub	r7, r4

	mov.w	@r5+, r2    /* Output offset */

	mov.w	@r5+, r1    /* Lines */
	shll16	r3

	xtrct	r0, r3

	mov.w	@r5+, r8    /* Input stride */
	mov	#8, r0      /* Maximum width for naive method */

	add	r6, r2
	cmp/ge	r7, r0

	bt.s	.naive
	mov	#2, r0

/* The following variations are named based on the parity of each parameter:
   * w[eo] (width even, width odd)
   * d[eo] (data even, data odd)
   where even/odd means 4-aligned/2-aligned in terms of pointers.

   When the destination and source have identical parity, the copy is pretty
   direct and takes 2 cycles to copy 4 bytes. When they have opposite parity
   however, longwords need to be rearranged, which is a problem: arithmetic
   operations under a RAW dependency take 3 cycles, so there's no way to
   complete the 4-byte copy in less than 4 cycles unless iterations are opened
   and weaved, which would add too much sub-cases. So in this case the naive
   method that copies 4 bytes in 4 cycles is used. A very heavy image renderer
   like a tileset shader should consider the optimized route though.  */

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
	rts;			\
	mov.l	@r15+, r8

.case_analysis:
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
