.global _azrp_image_shader_rgb16_dye
#define AZRP_RGB16_DYE
#include "image_rgb16_clearbg.S"

/* See image_rgb16_clearbg.S for details on this function. */

_azrp_image_shader_rgb16_dye:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_DYE_LOOP 0, 4, r14, r13, 2, 0, r0, r0
9:	GEN_CLEARBG_DYE_LOOP 1, -4, r13, r14, 0, 2, r0, r0
