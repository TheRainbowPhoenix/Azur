.global _azrp_text_glyph

/* Naive glyph rendering. Possible optimizations:
   - Use a 1-bit bitmask instead of a longword index? */

/* Parameters:
    r4:        fragment
    r5:        data
    r6:        color
    r7:        height
    @(4,r15):  width
    @(8,r15):  dataw - width (stride)
    @(12,r15): starting index in data
   Stack:
    @(0,r15): r8 save

   Register allocation:
    r0: (temporary)
    r1: (temporary)
    r2: x counter
    r3: glyph data index
    r4: fragment pointer
    r5: glyph pointer
    r6: color
    r7: y counter
   Callee-saved registers:
   r8: fragment stride */

_azrp_text_glyph:

	/* Compute fragment stride: 2 * (azrp_width-width) */
	mov.l	r8, @-r15
	mov.l	1f, r8
	mov.l	@r8, r8
	mov.l	@(4, r15), r3
	sub	r3, r8
	shll	r8

	/* Load the starting index */
	mov.l	@(12, r15), r3

.fg_y:
	/* Initialize width counter */
	mov.l	@(4, r15), r2

.fg_x:
	/* Load one bit of data in T */
	mov	r3, r0
	mov	#-5, r1
	shld	r1, r0
	shll2	r0
	mov.l	@(r0, r5), r1

	mov	r3, r0
	and	#31, r0

	shld	r0, r1
	shll	r1

	/* Write color to fragment only if it's a 1 bit */
	bf	.fg_next
	mov.w	r6, @r4

.fg_next:
	/* Leave the x-loop if x counter reaches 0 */
	add	#2, r4
	dt	r2
	bf/s	.fg_x
	add	#1, r3

	/* Move to next row, leave the y-loop if height reaches 0 */
	dt	r7
	mov.l	@(8, r15), r0
	add	r0, r3
	bf/s	.fg_y
	add	r8, r4

	rts
	mov.l	@r15+, r8

.align 4

1:	.long	_azrp_width
