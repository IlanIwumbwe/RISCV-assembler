#ifndef DEFS_H  
#define DEFS_H

#define FULL_REGEX R"([a-zA-Z_0-9]*|\-?0x[0-9a-fA-F]+|\-?[0-9]+|\(|\)|\.|,|:)"
#define R_TYPE_OPCODES R"(add|sub|sll|slt|sltu|xor|srl|sra|or|and|jalr)"
#define I_TYPE_OPCODES R"(lb|lh|lw|lbu|lhu|addi|slli|slti|sltiu|xori|srli|srai|ori|andi)"
#define S_TYPE_OPCODES R"(sb|sh|sw)"
#define B_TYPE_OPCODES R"(beq|bne|blt|bge|bltu|bgeu)"
#define U_TYPE_OPCODES R"(auipc|lui)"
#define J_TYPE_OPCODES R"(jal)"
#define REGISTERS R"(zero|ra|sp|gp|tp|t[0-6]|s[0-11]|fp|a[0-7]|x[0-31])"

#define t0to2 std::regex(R"(t[0-2])")
#define s0to1 std::regex(R"(s[0-1])")
#define a0to7 std::regex(R"(a[0-7])")
#define s2to11 std::regex(R"(s[2-11])")
#define t3to6 std::regex(R"(t[3-6])")
#define x_regnum std::regex(R"(x[0-31])")

#include <unordered_map>
#include <cstdint>
#include <cmath>

struct tableRow{
	uint32_t op;
	uint32_t funct3;
	uint32_t funct7;
};

class machine_codes{
	public:
		machine_codes(){}

		void setcodes(std::string opcode, const uint8_t& op, const uint8_t& funct3, const uint8_t& funct7) {
			tableRow row{op, funct3, funct7};

			codes[opcode] = row;
		}

		tableRow getcontrolbits(std::string opcode) {
			auto row = codes[opcode];
			return row;
		}

		uint32_t getspecialregnum(std::string reg){
			return reg_numbers[reg];
		}

		uint32_t getregnum(std::string reg){
		
			if(std::regex_match(reg, t0to2)){
				return (reg[1] - '0') + 5;
			} else if (std::regex_match(reg, s0to1)){
				return (reg[1] - '0') + 8;
			} else if (std::regex_match(reg, a0to7)){
				return (reg[1] - '0') + 10;
			} else if (std::regex_match(reg, s2to11)){
				return (reg[1] - '0') + 16;
			} else if (std::regex_match(reg, t3to6)) {
				return (reg[1] - '0') + 25;
			} else if (std::regex_match(reg, x_regnum)){
				return reg[1] - '0';
			} else {
				return 32;   // not a valid reg number, means we must check reg_numbers for special register name
			}
		}

	private:
		std::unordered_map<std::string, tableRow> codes = {
			{"add", {51, 0, 0}},
			{"sub", {51, 0, 64}},
			{"addi", {19, 0, 0}},
			{"bne", {99, 1, 0}}
		};

		std::unordered_map<std::string, uint32_t> reg_numbers = {
			{"zero", 0},
			{"ra", 1},
			{"s2", 2},
			{"gp", 3},
			{"tp", 4},
			{"fp", 8}
		};

};




#endif