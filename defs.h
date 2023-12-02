#ifndef DEFS_H  
#define DEFS_H

#ifdef _WIN32
const std::string pathseparator = "\\";
#else
const std::string pathseparator = "/";
#endif

#define FULL_REGEX R"([a-zA-Z_0-9]*|\-?0x[0-9a-fA-F]+|\-?[0-9]+|\(|\)|\.|,|:)"
#define R_TYPE_OPCODES R"(add|sub|sll|slt|sltu|xor|srl|sra|or|and)"
#define I_TYPE_OPCODES R"(lb|lh|lw|lbu|lhu|addi|slli|slti|sltiu|xori|srli|srai|ori|andi|jalr)"
#define S_TYPE_OPCODES R"(sb|sh|sw)"
#define B_TYPE_OPCODES R"(beq|bne|blt|bge|bltu|bgeu)"
#define U_TYPE_OPCODES R"(auipc|lui)"
#define J_TYPE_OPCODES R"(jal)"

#define SPECIAL_REGS std::regex(R"(zero|ra|sp|gp|tp|fp)")
#define t0to2 std::regex(R"(t[0-2])")
#define s0to1 std::regex(R"(s[0-1])")
#define a0to7 std::regex(R"(a[0-7])")
#define s2to11 std::regex(R"(s[2-9]|10|11)")
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

		/// This funtion needs to be redone, it can sometimes fail to work in mysterious ways
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
			{"sll", {51, 1, 0}},
			{"slt", {51, 2, 0}},
			{"sltu", {51, 3, 0}},
			{"xor", {51, 4, 0}},
			{"srl", {51, 5, 0}},
			{"sra", {51, 5, 64}},
			{"or", {51, 6, 0}},
			{"and", {51, 7, 0}},
			{"lb", {3, 0, 0}},
			{"lh", {3, 1, 0}},
			{"lw", {3, 2, 0}},
			{"lbu", {3, 4, 0}},
			{"lhu", {3, 5, 0}},
			{"addi", {19, 0, 0}},
			{"slli", {19, 1, 0}},
			{"slti", {19, 2, 0}},
			{"sltiu", {19, 3, 0}},
			{"xori", {19, 4, 0}},
			{"srli", {19, 5, 0}},
			{"srai", {19, 5, 64}},
			{"ori", {19, 6, 0}},
			{"andi", {19,7, 0}},
			{"sb", {35, 0, 0}},
			{"sh", {35, 1, 0}},
			{"sw", {35, 2, 0}},
			{"beq", {99, 0, 0}},
			{"bne", {99, 1, 0}},
			{"blt", {99, 4, 0}},
			{"bge", {99, 5, 0}},
			{"bltu", {99, 6, 0}},
			{"bgeu", {99, 7, 0}},
			{"auipc", {23, 0, 0}},
			{"lui", {55, 0, 0}},
			{"jalr", {103, 0, 0}},
			{"jal", {111, 0, 0}}
		};

		std::unordered_map<std::string, uint32_t> reg_numbers = {
			{"zero", 0},
			{"ra", 1},
			{"sp", 2},
			{"gp", 3},
			{"tp", 4},
			{"fp", 8}
		};

};




#endif