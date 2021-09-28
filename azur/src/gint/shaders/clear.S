/* Azur's built-in shaders: <clear>

   By far the easiest of all. The clear shader is a good benchmark for how fast
   the rendering pipeline is. And it's pretty damn fast, clocking in at 400 µs
   for a full-resolution 396x224 clear (compared to 6.1 ms for a VRAM clear by
   CPU or 2.5 ms by DMA).

   Because this is performed by CPU and therefore versatile, there are many
   variations with more complex patterns that will perform at the same speed.
   A gray tiled background for transparency in image viewing comes to mind, for
   example.

   Affected region: full-screen
   Asymptotic performance: 0.5 cycle/pixel */

.global _azrp_shader_clear
.align 4

_azrp_shader_clear:
	ldrs	1f
	ldre	2f
	ldrc	r4

	mov.w	@(2,r5), r0
	shll2	r4

	add	r4, r6

	swap.w	r0, r1

	xtrct	r0, r1

1: 2:	mov.l	r1, @-r6

	rts
	nop
