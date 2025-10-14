#pragma once

#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <filesystem>

#define set_bit(n) (1UL << n)

namespace fs = std::filesystem;

// colours
#define RED(x) (std::string("\033[31m") + x + std::string("\033[0m"))
#define YELLOW(x) (std::string("\033[33m") + x + std::string("\033[0m"))
#define GREEN(x) (std::string("\033[32m") + x + std::string("\033[0m"))

// location annotation
#define ANNOT(x) (std::string("at ") + __FILE__ + "," + std::to_string(__LINE__) + ": " + (x))

// logging
#define ERROR(x) std::cerr << (std::string("[ERROR] ") + RED(ANNOT(x))) << std::endl
#define WARNING(x) std::cout << (std::string("[WARNING] ") + YELLOW(ANNOT(x))) << std::endl
#define INFO(x) std::cout << (std::string("[INFO] ") + GREEN(x)) << std::endl

#define HEX(x) std::setfill('0') << std::setw(8) << std::hex << x << std::dec

using U32 = uint64_t;
using U64 = uint64_t;

namespace Assembler {

    enum Register_port {
        RD,
        RS1,
        RS2
    };

    struct Pseudo_instruction_data {
        U32 rd;
        U32 rs1;
        U32 rs2;
        U64 imm; 
    };

    struct Instruction_data {

        public:

            Instruction_data(){}

            Instruction_data(U32 _opcode, U32 _funct3, U32 _funct7) :
                opcode(_opcode),
                funct3(_funct3),
                funct7(_funct7)
            {}

            U32 get_opcode() const { return opcode; }

            U32 get_funct3() const { return funct3; }

            U32 get_funct7() const { return funct7; }

            void set_pseudo_instr_data(Pseudo_instruction_data data){
                psi_data = std::make_optional<Pseudo_instruction_data>(data);
            }

            friend std::ostream& operator<<(std::ostream& stream, const Instruction_data& data){
                stream << "opcode: " << data.opcode << " funct3: " << data.funct3 << " funct7: " << data.funct7;
                return stream;
            }

            bool operator==(const Instruction_data& other) const {
                return (opcode == other.opcode) && (funct3 == other.funct3) && (funct7 == other.funct7);
            }

        private:
            U32 opcode;
            U32 funct3;
            U32 funct7;

            std::optional<Pseudo_instruction_data> psi_data;

    };

    inline void to_file(std::ofstream& stream, const U32& num){
        stream << HEX(num) << std::endl;
    }


    inline U32 get_bits(U32 source, int msb, int lsb) {
        int width = msb - lsb + 1;
        U32 mask = (1U << width) - 1; // for width 8, this would be OxFF
        return (source >> lsb) & mask;
    }


    inline void place_bits(U32& target, U32 field_value, int low_bit, int width) {
        // almost the same as `mask` above. Just shifted in the target position, then negated
        U32 clearing_mask = ~(((1U << width) - 1) << low_bit);
        
        target &= clearing_mask;
        
        // place value at target position
        target |= (field_value << low_bit);
    }

}