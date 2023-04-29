# --------------------------------------------------------------------------- #
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
# --------------------------------------------------------------------------- #
# SH4AL-DSP optimized i32 / i16 -> i16 division.
#
# This simply uses the CPU's ability to divide without rotation for 16 bit
# divisors without all the boilerplate than libgcc's __sdivisi3 requires since
# it assumes 32-bit inputs. Used for num16 division.
# ---

.global __ZN6libnum4prim15div_i32_i16_i16Els

# libnum::prim::div_i32_i16_i16(long, short)
__ZN6libnum4prim15div_i32_i16_i16Els:
	shll16	r5
	mov	#0, r2

	mov	r4, r3
	rotcl	r3

	subc	r2, r4

	div0s	r5, r4

	div1	r5, r4
	div1	r5, r4
	div1	r5, r4
	div1	r5, r4

	div1	r5, r4
	div1	r5, r4
	div1	r5, r4
	div1	r5, r4

	div1	r5, r4
	div1	r5, r4
	div1	r5, r4
	div1	r5, r4

	div1	r5, r4
	div1	r5, r4
	div1	r5, r4
	div1	r5, r4

	exts.w	r4, r4

	rotcl	r4

	addc	r2, r4

	rts
	exts.w	r4, r0
