# RISC-V Assembler

Compile using `make`

Run the assember using `./assembler -p <FILE-PATH>`

File path can be a single assembly file (`.asm` or `.s` extensions) or a folder containing assembly files. In the second case, each file is assembled one at a time.

Machine code will be found in same directory as input file(s), with the same name as the input file but a `.txt` extension.

Write a program in RISC-V assembly and convert it into machine code!

### To-do
- [x] Conversion for psuedo instructions
- [ ] Correctly interpret all directives (only `.equ` affects the program, the rest (`.global`, `.text`) are just printed out.)
- [x] The code is wrong when you work with a folder of assembly files. Working with single files works. Need to fix this. 
- [ ] Add interpretation for binary immediates for I type instructions
