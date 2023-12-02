# RISC-V Assembler

Compile using `make`

Run the assember using `./assembler -p <FILE-PATH>`

File path can be a single assembly file (`.asm` or `.s` extensions) or a folder containing assembly files. In the second case, each file is assembled one at a time.

Machine code will be found in same directory as input file(s), with the same name as the input file but a `.txt` extension.

Write a program in RISC-V assembly (no psudo instructions), and convert it into machine code!

### To-do
- [ ] Convertion for psuedo instructions
- [ ] Correctly intepret all directives (only `.equ` affects the program, the rest (`.global`, `.text`) are just printed out.)