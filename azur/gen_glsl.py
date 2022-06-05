#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# gen_glsl.py: Trivial utility to embed GLSL files into the library
#
# Azur comes with a set of default shaders, but installing GLSL files to be
# loaded at runtime comes with a number of install/distribution constraints.
# For simplicity, we embed the files into the library by generating a C file
# with a bunch of strings and compiling it.
#---

import sys
import os

if len(sys.argv) < 2 or "-h" in sys.argv or "--help" in sys.argv:
    print(f"usage: {sys.argv[0]} <OUTPUT.c> <INPUT.glsl...>")

def encode(string):
    # Escape characters that have special meaning in C
    string = string.replace("\\", "\\\\").replace("\"", "\\\"")
    # Enforce proper newlines
    return "\n".join(f'"{s}\\n"' for s in string.split("\n"))

inputs = []
for input_file in sys.argv[2:]:
    with open(input_file, "r") as fp:
        name = os.path.splitext(os.path.basename(input_file))[0]
        inputs.append((name, fp.read()))

with open(sys.argv[1], "w") as fp:
    for (name, data) in inputs:
        fp.write(f"char const *azur_glsl__{name} =\n{encode(data)};\n\n")
