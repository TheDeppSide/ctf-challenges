# 📝 ORION Challenge — Author Notes

This file is **NOT for players**. It documents how the challenge is designed, how the language works, and what we ship.

---

## 1. Concept
- We provide participants with:
  - A **meme-style README** (alien transmission, very minimal).
  - The file `program.orion` that prints the flag.
  - Optionally some small sample programs (sanity checks).
- Players must **reverse-engineer the language** by trial/error on the samples, then write an interpreter to run `program.orion`.

---

## 2. Language Specification (full)
- **Memory:**
  - Two tapes `T0`, `T1`, each of length 65,536.
  - Each cell is 1 byte (0–255, wraps).
  - Two pointers `P0`, `P1`, starting at 0.
  - Tape selector `S` decides active tape/pointer (starts at 0).

- **Instructions:**
  - `>` : increment active pointer.
  - `<` : decrement active pointer.
  - `+` : increment active cell (wrap 0–255).
  - `-` : decrement active cell (wrap 0–255).
  - `.` : output active cell as a byte.
  - `[` / `]` : loops (Brainfuck style).
  - `~` : toggle tape selector (switch between T0/T1).
  - `=HH` : set current cell to hex literal `HH`.
  - `^HH` : XOR current cell with hex literal `HH`.
  - `@` : copy burst. Copy `N = cell` bytes from active tape to inactive tape (starting at their pointers). Pointers unchanged.
  - `` `HH `` : flip window. For the next `K=HH` `+`/`-` operations, flip their meaning. Multiple windows stack.

- **Repeat macros:**
  - A hex number immediately before a repeatable instruction (like `A+`, `10.`, `F@`) means repeat N (in hex) times.

- **Comments:**
  - `#` starts a comment until end of line.
  - Any unrecognized character = no-op.

---

## 3. Input/Output policy
- No keyboard or stdin input. `,` is always EOF (no effect).
- Programs output only through `.`.
- Execution must terminate; timeouts prevent infinite loops.

---

## 4. Files distributed to players
- `README.md` (meme/alien minimal).
- `program.orion` (offuscated file printing the flag).
- *(optional)* `samples/` folder with:
  - `hello.orion` → prints `CTF{HI}`.
  - `copy.orion` → demonstrates `@`.
  - `flip.orion` → demonstrates `` `HH ``.

---

## 5. Flag
- Format: `ptm{...}`.
- Example used:  ptm{wh0_n33d5_asm_64_when_u_h4v3_w31rd_lang5_2_p0w3r_ur_d15c0rd_b0t_24_7}
---

## 6. Obfuscation guidelines
- Use heavy comments, dead `[ ... ]` blocks.
- Sprinkle neutral ops (`0+0-`, `^FF^FF`, `` `00 ``).
- Insert pointer jitter (`10>10<`).
- Spread output across both tapes using `~` and `@` if desired.
- Always keep the final program under a reasonable token budget (e.g., < 30k tokens).

---

## 7. Checking solution
- Keep the **reference interpreter** (Python, ~300 LOC) private.
- Use it in the checker to run `program.orion`.
- Compare stdout with expected flag string.

---

## 8. Pitfalls for players
- Must notice there are **two tapes**.
- Must handle **flip windows** correctly (stacking).
- Must parse **multi-char tokens** (`=HH`, `^HH`, `` `HH ``).
- Must implement **hex repeat macros**.
- Must handle comments and ignore unknown chars.
- Must allow wrapping of pointers (16-bit) and cells (8-bit).

---

## 9. Anti-cheese notes
- Because input is disabled, players cannot “cheat” by feeding files.
- The only way to get the flag is to implement the interpreter properly.
- If desired, can add trivial decoys inside `program.orion` (printing junk before flag).

---

*This document is for authors only. Do not release to participants.*