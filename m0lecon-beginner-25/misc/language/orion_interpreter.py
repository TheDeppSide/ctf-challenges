#!/usr/bin/env python3
"""
ORION esolang — reference interpreter (Python 3)

Spec summary:
- Two uint8 tapes T0/T1 of length 65536 (wrap at 256 for cells, 65536 for pointers)
- Pointers P0/P1; tape selector S (0 => T0/P0, 1 => T1/P1)
- Instructions: > < + - . , [ ] ~ @ =HH ^HH `HH
- Repeat macros: 1-4 hex digits immediately before a repeatable op (> < + - . , ~ @) repeat it N times (hex)
- Tokens =HH, ^HH, `HH are 3-char each and count as one instruction
- ',' at EOF leaves the cell unchanged
- Flip window (`HH): for the next K=0xHH effective +/- steps, swap + and - (multiple windows stack; parity matters)

Usage:
  python3 orion_interpreter.py program.orion [--input path]
  # or
  python3 orion_interpreter.py program.orion < input.bin
"""
from __future__ import annotations
from dataclasses import dataclass
from typing import List, Optional, Dict, Tuple
import sys, io, argparse

REPEATABLE = set('><+-.,~@')
HEX_CHARS = set('0123456789abcdefABCDEF')

@dataclass
class Token:
    op: str                 # 'GT','LT','PLUS','MINUS','DOT','COMMA','LBR','RBR','TILDE','COPY','EQUAL','XOR','FLIP'
    arg: Optional[int] = None  # for EQUAL/XOR/FLIP (0..255)
    count: int = 1             # repetition count for repeatable ops


def is_hex(ch: str) -> bool:
    return ch in HEX_CHARS

def hex_to_int(s: str) -> int:
    return int(s, 16)


def char_to_op(ch: str) -> str:
    return {
        '>': 'GT', '<': 'LT', '+': 'PLUS', '-': 'MINUS', '.': 'DOT', ',': 'COMMA',
        '~': 'TILDE', '@': 'COPY'
    }[ch]


def tokenize(code: str) -> Tuple[List[Token], Dict[int, int]]:
    """Tokenize source and build jump table for [ ]."""
    tokens: List[Token] = []
    i = 0
    n = len(code)
    while i < n:
        c = code[i]

        # Line comment starting with '#'
        if c == '#':
            j = i + 1
            while j < n and code[j] != '\n':
                j += 1
            i = j + 1
            continue

        # 3-char tokens: =HH, ^HH, `HH
        if c in ('=', '^', '`'):
            if i + 2 < n and is_hex(code[i+1]) and is_hex(code[i+2]):
                val = hex_to_int(code[i+1:i+3]) & 0xFF
                if c == '=':
                    tokens.append(Token('EQUAL', arg=val))
                elif c == '^':
                    tokens.append(Token('XOR', arg=val))
                else:
                    tokens.append(Token('FLIP', arg=val))
                i += 3
                continue
            else:
                # Unrecognized pattern -> skip this char
                i += 1
                continue

        # Hex-prefix macro for repeatable ops (1–4 nibbles)
        if is_hex(c):
            j = i
            k = 0
            while j < n and is_hex(code[j]) and k < 4:
                j += 1
                k += 1
            if j < n and code[j] in REPEATABLE:
                count = hex_to_int(code[i:j])
                inst = code[j]
                if count > 0:
                    tokens.append(Token(char_to_op(inst), count=count))
                i = j + 1
                continue
            else:
                # Hex digits not followed by a repeatable op → ignore
                i = j
                continue

        # Single glyphs
        if c in '><+-.,[]~@':
            if c in REPEATABLE:
                tokens.append(Token(char_to_op(c), count=1))
            elif c == '[':
                tokens.append(Token('LBR'))
            elif c == ']':
                tokens.append(Token('RBR'))
            i += 1
            continue

        # Anything else -> no-op
        i += 1

    # Build jump table for brackets on token indices
    jump: Dict[int, int] = {}
    stack: List[int] = []
    for idx, tok in enumerate(tokens):
        if tok.op == 'LBR':
            stack.append(idx)
        elif tok.op == 'RBR':
            if not stack:
                raise SyntaxError(f"Unmatched ']' at token index {idx}")
            j = stack.pop()
            jump[idx] = j
            jump[j] = idx
    if stack:
        raise SyntaxError(f"Unmatched '[' at token index {stack[-1]}")

    return tokens, jump


def run(tokens: List[Token], jump: Dict[int, int], data_in: bytes = b'') -> bytes:
    # State
    T0 = bytearray(65536)
    T1 = bytearray(65536)
    P0 = 0
    P1 = 0
    S = 0  # 0 => T0/P0, 1 => T1/P1

    ip = 0
    out = bytearray()
    inbuf = io.BytesIO(data_in)

    # Active flip windows: list of remaining counts; parity(len) decides flip
    flip_windows: List[int] = []

    while ip < len(tokens):
        tok = tokens[ip]
        op = tok.op

        # Handle brackets first (count is always 1 for brackets)
        if op == 'LBR':
            v = T0[P0] if S == 0 else T1[P1]
            if v == 0:
                ip = jump[ip] + 1
            else:
                ip += 1
            continue
        if op == 'RBR':
            v = T0[P0] if S == 0 else T1[P1]
            if v != 0:
                ip = jump[ip] + 1
            else:
                ip += 1
            continue

        reps = tok.count if op in ('GT','LT','PLUS','MINUS','DOT','COMMA','TILDE','COPY') else 1
        for _ in range(reps):
            if op == 'GT':
                if S == 0: P0 = (P0 + 1) & 0xFFFF
                else:      P1 = (P1 + 1) & 0xFFFF
            elif op == 'LT':
                if S == 0: P0 = (P0 - 1) & 0xFFFF
                else:      P1 = (P1 - 1) & 0xFFFF
            elif op in ('PLUS','MINUS'):
                # Determine effective operation based on current flip parity
                parity = (len(flip_windows) & 1)
                is_plus = (op == 'PLUS')
                eff_plus = is_plus ^ (parity == 1)
                if S == 0:
                    T0[P0] = ((T0[P0] + 1) & 0xFF) if eff_plus else ((T0[P0] - 1) & 0xFF)
                else:
                    T1[P1] = ((T1[P1] + 1) & 0xFF) if eff_plus else ((T1[P1] - 1) & 0xFF)
                # Decrement all active flip windows by 1; remove expired
                if flip_windows:
                    new_fw = []
                    for k in flip_windows:
                        k -= 1
                        if k > 0:
                            new_fw.append(k)
                    flip_windows = new_fw
            elif op == 'DOT':
                v = T0[P0] if S == 0 else T1[P1]
                out.append(v)
            elif op == 'COMMA':
                b = inbuf.read(1)
                if b:
                    if S == 0: T0[P0] = b[0]
                    else:      T1[P1] = b[0]
            elif op == 'TILDE':
                S ^= 1
            elif op == 'COPY':
                N = T0[P0] if S == 0 else T1[P1]
                if N:
                    if S == 0:
                        src, sp = T0, P0
                        dst, dp = T1, P1
                    else:
                        src, sp = T1, P1
                        dst, dp = T0, P0
                    for k in range(N):
                        dst[(dp + k) & 0xFFFF] = src[(sp + k) & 0xFFFF]
            elif op == 'EQUAL':
                v = tok.arg & 0xFF
                if S == 0: T0[P0] = v
                else:      T1[P1] = v
            elif op == 'XOR':
                v = tok.arg & 0xFF
                if S == 0: T0[P0] ^= v
                else:      T1[P1] ^= v
            elif op == 'FLIP':
                k = tok.arg & 0xFF
                if k:
                    flip_windows.append(k)
            else:
                # Shouldn't happen
                pass
        ip += 1

    return bytes(out)


def main() -> None:
    ap = argparse.ArgumentParser(description='ORION esolang — reference interpreter')
    ap.add_argument('program', help='ORION source file (.orion)')
    args = ap.parse_args()

    try:
        code = open(args.program, 'r', encoding='utf-8', errors='ignore').read()
    except OSError as e:
        print(f"Error reading program: {e}", file=sys.stderr)
        sys.exit(1)


    try:
        tokens, jump = tokenize(code)
    except SyntaxError as e:
        print(f"Syntax error: {e}", file=sys.stderr)
        sys.exit(2)

    out = run(tokens, jump)
    sys.stdout.buffer.write(out)


if __name__ == '__main__':
    main()


# ptm{wh0_n33d5_asm_64_when_u_h4v3_w31rd_lang5_2_p0w3r_ur_d15c0rd_b0t_24_7}