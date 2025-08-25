#! /usr/bin/env python
#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# gen-assets.py: Asset group generator

import itertools
import getopt
import sys
import re
import os

USAGE = """\
usage: gen-assets.py [OPTIONS...] -- <INPUTS...>
Generates an asset group into a C++ file for one or more files or folders.

Options:
  -o <OUTPUT>     Output file [-]
  -n <NAME>       Asset group name; should really be given [test]

Inputs (must come after "--"):
  -d <FOLDER>     Interpret incoming paths relative to this folder
  <PATH>          Take this file (relative to last given -d option)
"""

#=== Utilities ================================================================#

def itertools_batched(iterable, n):
    # batched('ABCDEFG', 2) → AB CD EF G
    if n < 1:
        raise ValueError('n must be at least one')
    iterator = iter(iterable)
    while batch := tuple(itertools.islice(iterator, n)):
        yield batch

def C_string(s: str) -> str:
    s = s.encode("unicode_escape").replace(b'"', b'\\"').decode("ascii")
    return '"' + s + '"'

def is_valid_name(name: str) -> bool:
    if name in ["azur"]:
        print(f"error: asset group name {name!r} is reserved")
        return False
    if not re.fullmatch(r'[a-zA-Z_][a-zA-Z0-9_]*', name):
        print(f"error: asset group name {name!r} is not a valid C identifier")
        return False
    return True

#=== Package group generation =================================================#

def write_file_data(fp, id, data):
    fp.write(f"static const unsigned char __file_{id}[{len(data)}] = {{\n")
    for line in itertools_batched(data, 12):
        fp.write("  ")
        fp.write(" ".join(f"0x{b:02x}," for b in line))
        fp.write("\n")
    fp.write("};\n\n")

def write_index(fp, index: list[tuple[str, int, int]]) -> None:
    fp.write("static azur::ResourceGroup __index {\n")
    for path, id, size in index:
        path_str = C_string(path)
        fp.write(f"  {{{path_str}, {{__file_{id}, {size}}}}},\n")
    fp.write("};\n\n")

def write_registration(fp, group_name: str) -> None:
    group_name_str = C_string(group_name)
    fp.write("__attribute__((constructor))\n")
    fp.write("static void __register() {\n");
    fp.write(f"  azur::registerResourceGroup({group_name_str}, __index);\n")
    fp.write("}\n")

#=== Argument parsing =========================================================#

def parse_cli_args():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "o:n:", ["help"])
    except getopt.GetoptError as e:
        print("error:", e, file=sys.stderr)
        print(f"Try '{sys.argv[0]} --help' for more information.")
        sys.exit(1)

    if len(sys.argv) == 1 or ("--help", "") in opts:
        print(USAGE, end="", file=sys.stderr)
        sys.exit(0)

    return dict(opts), args

def main(argv):
    opts, args = parse_cli_args()
    output = opts.get("-o", "-")
    group_name = opts.get("-n", "test")

    if not is_valid_name(group_name):
        sys.exit(1)

    index = []

    # TODO: Check that index does not have any duplicate paths!
    fp = open(output, "w") if output != "-" else sys.stdout
    with fp as fp:
        fp.write("#include <azur/resources.h>\n\n")

        i = 0
        id = 0
        current_folder = "."
        while i < len(args):
            if args[i] == "-d":
                if i + 1 < len(args):
                    current_folder = args[i + 1]
                    i += 1
            else:
                path = args[i]
                relative_path = os.path.relpath(path, current_folder)

                with open(path, "rb") as fp_bin:
                    data = fp_bin.read()

                write_file_data(fp, id, data)
                index.append((relative_path, id, len(data)))
                id += 1
                if output != "-":
                    print(f"Embedding @{group_name}:{relative_path}")
            i += 1

        write_index(fp, index)
        write_registration(fp, group_name)

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))

