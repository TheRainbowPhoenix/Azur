#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# isel.py: Instruction selection testing facility
#
# Many of the primitive numerical functions of libnum are written to compile
# efficiently on SuperH, where "efficiently" mainly cares about using specific
# instructions and generating short code. This utility tests these assumptions
# by compiling C++ files that use the library, and matching the assembler code
# against simple specifications.
#
# The assembler output from g++ is decoded by simple text analysis. Function
# span from their `_SYMBOL:` to their `.size _SYMBOL, .-_SYMBOL`. Mnemonics are
# split on whitespace and arguments on commas; directives and unused labels are
# removed.
#
# The assembler output is matched using two types of expressions:
# - "Program expressions", which run against an entire function and evaluate to
#   integers. These are used to count, mainly.
# - "Instruction expressions", which run against single instructions and
#   evaluate to booleans. These are used to identify instructions.
#
# Instruction expressions are based on wildcard patterns matching mnemonics,
# like `add*` or `b?.s`, combined with logical operators `!`, `&&` and `||`.
# For instance, `add || addc || addv` identifies addition instructions, while
# `!mov && !rts` eliminates the usual function boilerplate.
#
# Program expressions are based on the count-expression `[e]` where `e` is an
# instruction expression. `[e]` evaluates to the number of instructions in the
# function that match `e`. These are combined with integral constants and the
# usual arithmetic, logical and comparison operators. C-style bool-as-int
# semantics are used, so tests like `!= 0` or `> 0` can often be omitted. `%`
# is a shortcut for the number of "non-trivial" instructions, and currently
# expands to `[!mov && !rts]`.
#
# A test is a normal C++ source built using the library, which exposes
# functions with C linkage (ie. no name mangling) and has specifications in
# comments of the form:
#
#  // FUNCTION_NAME: PROGRAM_EXPRESSION
#
# The comments should span entire lines and need not be placed near the
# functions that they test.
#---

from dataclasses import dataclass
import subprocess
import functools
import fnmatch
import typing
import enum
import sys
import os
import re

#---
# Program representation and parsing
#---

@dataclass
class Insn:
    """A concrete instruction from a compiled assembly program."""
    mnemonic: str
    args: list[str]

#---
# Specification representation and parsing
#---

@dataclass
class InsnPattern:
    """An abstract pattern that can be matched against asm instructions."""

    # Instruction mnemonic as a wildcard pattern (may use '?' and '*')
    mnemonicWildcard: str
    # Pattern for the arguments
    # args: list

    def evalAtInsn(self, program, i) -> bool:
        return fnmatch.fnmatchcase(program[i].mnemonic, self.mnemonicWildcard)

    def treeStr(self, indent):
        return (" "*indent) + self.mnemonicWildcard

@dataclass
class InsnExpr:
    """An expression built off `InsnPattern`, which runs on single instructions
       and evaluates to a boolean."""

    # Node constructor and arguments
    ctor: str
    args: list[typing.Union[InsnPattern, "InsnExpr"]]

    def evalAtInsn(self, program, i) -> bool:
        args = [a.evalAtInsn(program, i) for a in self.args]

        match self.ctor, args:
            case "mnemonic", [x]: return x
            # Boolean operators
            case "!",  [x]:   return 1 if not x else 0
            case "&&", [x,y]: return 1 if x and y else 0
            case "||", [x,y]: return 1 if x or y else 0

    def treeStr(self, indent):
        args = [a.treeStr(indent+2) for a in self.args]
        return (" "*indent) + self.ctor + ":\n" + "\n".join(args)

@dataclass
class ProgExpr:
    """An expression which runs on full programs and evaluates to counts of
       matched instructions."""

    # Node constructor and arguments
    ctor: str
    args: list[typing.Union[InsnExpr, int, "Expr"]]

    def evalArg(self, program, arg):
        match arg:
            case int(i):
                return i
            case InsnExpr(_, _) as e:
                return sum(int(e.evalAtInsn(program, i))
                           for i in range(len(program)))
            case ProgExpr(_, _) as e:
                return e.evalAtProg(program)

    def evalAtProg(self, program) -> int:
        args = [self.evalArg(program, a) for a in self.args]

        match self.ctor, args:
            case "atom", [x]: return x
            # Comparisons
            case "<",  [x,y]: return x < y
            case ">",  [x,y]: return x > y
            case "<=", [x,y]: return x <= y
            case ">=", [x,y]: return x >= y
            case "=",  [x,y]: return x == y
            case "!=", [x,y]: return x != y
            # Arithmetic
            case "+",  [x,y]: return x + y
            case "-",  [x,y]: return x - y
            case "-",  [x]:   return -x
            # Boolean operators
            case "!",  [x]:   return 1 if not x else 0
            case "&&", [x,y]: return 1 if x and y else 0
            case "||", [x,y]: return 1 if x or y else 0

    def argStr(self, arg, indent):
        match arg:
            case int(i):
                return (" "*indent) + str(i)
            case InsnExpr(_, _) as e:
                return e.treeStr(indent)
            case ProgExpr(_, _) as e:
                return e.treeStr(indent)

    def treeStr(self, indent):
        args = [self.argStr(a, indent+2) for a in self.args]
        return (" "*indent) + self.ctor + ":\n" + "\n".join(args)

T = enum.Enum("T", "NUM OP MNEMONIC LPAR RPAR LBRA RBRA PERCENT END".split())

@dataclass
class Token:
    type: T
    value: typing.Any

    def __str__(self):
        return f"{self.type}({self.value})"

class ExprLexer:
    """A lexer for `InsnPattern`, `InsnExpr` and `ProgExpr`."""

    RE_NUM       = re.compile(r"[0-9]+|0[xX][0-9a-fA-F]+")
    RE_OP        = re.compile(r"!|&&|\|\||<=|>=|<>|<|>|=|!=|\+|-")
    RE_MNEMONIC  = re.compile(r"[a-zA-Z.*?][a-zA-Z0-9.*?]*")
    PUNCT = {
        "(": T.LPAR, ")": T.RPAR,
        "[": T.LBRA, "]": T.RBRA,
        "%": T.PERCENT,
    }

    def __init__(self, code):
        """Initialize the lexer to start analyzing `code`."""
        self.sourceCode = code
        self.rewind()

    def rewind(self):
        """Start or restart lexing the same input."""
        self.code = self.sourceCode
        self.position = 0
        self.errors = 0

    def atEnd(self):
        """Check whether the end of the input has been reached."""
        return len(self.code) == 0

    def dump(self, fp=sys.stdout):
        """Exhaust lexer input and print the result to the specified stream."""
        while not self.atEnd():
            t = self.lex()
            print(f"{self.position:5d}: {t}", file=fp)

    def lex(self):
        """Return the next token in the stream."""
        self.position += 1
        c = self.code.lstrip(" \t")

        if len(c) == 0:
            return Token(T.END, None)

        if c[0] in ExprLexer.PUNCT:
            self.code = c[1:]
            return Token(ExprLexer.PUNCT[c[0]], None)

        if (m := ExprLexer.RE_NUM.match(c)) is not None:
            self.code = c[len(m[0]):]
            return Token(T.NUM, int(m[0], 0))

        if (m := ExprLexer.RE_OP.match(c)) is not None:
            self.code = c[len(m[0]):]
            return Token(T.OP, m[0])

        if (m := ExprLexer.RE_MNEMONIC.match(c)) is not None:
            self.code = c[len(m[0]):]
            return Token(T.MNEMONIC, m[0])

        # Raise a lexing error
        s = c.split(maxsplit=1)
        err = s[0]
        self.code = s[1] if len(s) > 1 else ""
        raise Exception(f"Lexing error near '{err}'")

class ExprParser:
    """An LL(1) recursive descent parser for program expressions."""

    def __init__(self, lexer):
        """Parse the output of a given lexer."""
        self.lexer = lexer
        self.la = None

    def parseProgExpr(self):
        """Parse the entire input as a ProgExpr."""
        self.lexer.rewind()
        self.la = None
        self.advance()
        e = self.pexpr()

        if not self.lexer.atEnd():
            print("Remaining input:")
            self.lexer.dump()
            raise Exception("Syntax error: expected end of input")
        return e

    def advance(self):
        """Return the next token and update the lookahead."""
        next = self.la
        self.la = self.lexer.lex()
        return next

    def expect(self, types, pred=None, optional=False):
        """Read the next token, ensuring it is one of the specified types; if
           `pred` is specified, also tests the predicate. If `optional` is set,
           returns None in case of mismatch rather than raising an error."""

        if isinstance(types, T):
            types = [types]
        if self.la.type in types and (pred is None or pred(self.la)):
            return self.advance()
        if optional:
            return None

        expected = ", ".join(str(t) for t in types)
        pos = self.lexer.position
        err = f"Expected one of {expected}, got {self.la} (at token {pos})"
        if pred is not None:
            err += " (with predicate)"
        raise Exception(f"Syntax error: {err}")

    # Rule combinators implementing unary and binary operators with precedence

    def binaryOpsLeft(ctor, ops):
        def decorate(f):
            def symbol(self):
                e = f(self)
                pred = lambda t: t.value in ops
                while (op := self.expect(T.OP, pred, True)) is not None:
                    e = ctor(op.value, [e, f(self)])
                return e
            return symbol
        return decorate

    def binaryOps(ctor, ops, *, rassoc=False):
        def decorate(f):
            def symbol(self):
                lhs = f(self)
                pred = lambda t: t.value in ops
                if (op := self.expect(T.OP, pred, True)) is not None:
                    rhs = symbol(self) if rassoc else f(self)
                    return ctor(op.value, [lhs, rhs])
                else:
                    return lhs
            return symbol
        return decorate

    def binaryOpsRight(ctor, ops):
        return binaryOpsRight(ctor, ops, rassoc=True)

    def unaryOps(ctor, ops, assoc=True):
        def decorate(f):
            def symbol(self):
                if (op := self.expect(T.OP, optional=True,
                        pred=lambda t: t.value in ops)) is not None:
                    arg = symbol(self) if assoc else f(self)
                    return ctor(op.value, [arg])
                else:
                    return f(self)
            return symbol
        return decorate

    # Parsing rules

    @binaryOpsLeft(ProgExpr, ["||"])
    @binaryOpsLeft(ProgExpr, ["&&"])
    @binaryOps(ProgExpr, [">", ">=", "<", "<=", "=", "!="])
    @binaryOpsLeft(ProgExpr, ["+", "-"])
    @unaryOps(ProgExpr, ["!", "-"])
    def pexpr(self):
        t = self.expect([T.LPAR, T.NUM, T.PERCENT, T.LBRA])
        if t.type == T.LPAR:
            pe = self.pexpr()
            self.expect(T.RPAR)
            return pe
        elif t.type == T.NUM:
            return ProgExpr("atom", [t.value])
        elif t.type == T.PERCENT:
            return parseProgExpr("[!rts && !mov]")
        elif t.type == T.LBRA:
            ie = self.iexpr()
            self.expect(T.RBRA)
            return ProgExpr("atom", [ie])

    @binaryOpsLeft(InsnExpr, ["||"])
    @binaryOpsLeft(InsnExpr, ["&&"])
    @unaryOps(InsnExpr, ["!"])
    def iexpr(self):
        if self.expect(T.LPAR, optional=True):
            ie = self.iexpr()
            self.expect(T.RPAR)
            return ie
        else:
            ip = self.ipat()
            return InsnExpr("mnemonic", [ip])

    def ipat(self):
        t = self.expect(T.MNEMONIC)
        return InsnPattern(t.value)

def parseProgExpr(string):
    l = ExprLexer(string)
    p = ExprParser(l)
    return p.parseProgExpr()

#---
# Main logic
#---

def runCompiler(input, flags):
    p = subprocess.run(
        ["sh-elf-g++", input, *flags, "-S", "-o", "-", "-std=c++20", "-O2"],
        stdout=subprocess.PIPE, check=True)
    return str(p.stdout, "utf8")

def extractFunctions(asm):
    # Normalize spacing
    asm = asm.replace("\t", " ")
    # Split into lines and remove indentation
    lines = [l.strip() for l in asm.splitlines()]
    # Remove directives and local symbols
    lines = [l for l in lines if l and not l.startswith(".")]

    funcs = dict()
    currentFunc = None

    for l in lines:
        if l.endswith(":"):
            currentFunc = l[:-1]
            funcs[currentFunc] = []
        elif currentFunc is None:
            raise Exception(f"instruction '{l}' before symbol name")
        else:
            mnemonic, *args = l.split(maxsplit=1)
            if args != []:
                args = [a.strip() for a in args[0].split(",")]
            funcs[currentFunc].append(Insn(mnemonic, args))

    return funcs

def printRawFunction(asm, sybl):
    # Find symbol definition
    start = asm.index(sybl + ":")
    if start < 0:
        print(f"<Unable to extract function {sybl}>")
        return False

    # Find function size
    end = asm.index(f".size\t{sybl}, .-{sybl}", start)
    if end < 0:
        print(f"<Unable to extract function {sybl}")
        return False

    func = asm[start:end].strip() + "\n"

    # Eliminate labels that are defined but unused
    RE_LABEL = re.compile(r"^(\.[a-zA-Z_][a-zA-Z0-9_]*):$", re.MULTILINE)
    for label in RE_LABEL.findall(func):
        if func.count(label) == 1:
            func = func.replace(f"{label}:\n", "")

    print(func.strip())
    return True

def loadTests(input):
    RE_SPEC = re.compile(r'^//\s*([a-zA-Z_][a-zA-Z0-9_]*):\s*(.+)$')

    with open(input, "r") as fp:
        code = fp.read()

    tests = dict()
    for line in code.split("\n"):
        m = RE_SPEC.match(line)
        if m is not None:
            sybl = "_" + m[1]
            if sybl not in tests:
                tests[sybl] = []
            tests[sybl].append((m[2], parseProgExpr(m[2])))

    return tests

if len(sys.argv) < 2:
    print(f"usage: {sys.argv[0]} <C++ SOURCE> <CXXFLAGS...>", file=sys.stderr)

asm = runCompiler(sys.argv[1], sys.argv[2:])
funcs = extractFunctions(asm)
tests = loadTests(sys.argv[1])
errors = False

for sybl in tests:
    if sybl not in funcs:
        print(f"error: no function '{sybl}' found", file=sys.stderr)
        errors = True

init = True
for sybl in sorted(tests):
    if not init:
        print("")
    init = False
    print(f"\x1b[36m{40*'<>'}\x1b[0m")

    if not printRawFunction(asm, sybl):
        errors = True
    print("")

    for ref, expr in tests[sybl]:
        r = expr.evalAtProg(funcs[sybl])
        if r != 0:
            print(f"\x1b[32mPASSED\x1b[0m  {ref}")
        else:
            print(f"\x1b[31mFAILED\x1b[0m  {ref}")
            errors = True

sys.exit(1 if errors else 0)
