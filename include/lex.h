#pragma once

#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <variant>
#include <utils.h>

#include "string.h"

namespace Assembler {

    enum Token_kind {
        _EOF,

        /*
            base instructions start here
        */
        BASE_INSTR_START,

        /*
            R_TYPE
        */
        R_TYPE_START,
        ADD,
        SUB,
        XOR,
        OR,
        AND,
        SLL,
        SRL,
        SRA,
        SLT,
        SLTU,
        R_TYPE_END,
        /*
            I_TYPE
        */
        I_TYPE_START,
        ADDI,
        SLLI,
        SLTI,
        SLTIU,
        XORI,
        ORI,
        ANDI,
        SRLI,
        SRAI,
        LB,
        LH,
        LW,
        LBU,
        LHU,
        JALR,
        I_TYPE_END,
        /*
            S_TYPE
        */
        S_TYPE_START,
        SB,
        SH,
        SW,
        S_TYPE_END,
        /*
            B_TYPE
        */
        B_TYPE_START,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        B_TYPE_END,
        /*
            U_TYPE
        */
        U_TYPE_START,
        LUI,
        AUIPC,
        U_TYPE_END,
        /*
            J_TYPE
        */
        J_TYPE_START,
        JAL,
        J_TYPE_END,

        /*
            base instructions end here
        */
        BASE_INSTR_END,

        /*
            psuedo instructions start here
        */
        PSEUDO_INSTR_START,
        LI,
        LA,
        MV,
        NOT,
        NEG,
        BGT,
        BLE,
        BGTU,
        BLEU,
        BEQZ,
        BNEZ,
        BGEZ,
        BLEZ,
        BGTZ,
        J,
        CALL,
        RET,
        NOP,
        /*
            psuedo instructions end here
        */
        PSEUDO_INSTR_END,
        /*
            Registers
        */
        REG_BEGIN,
        X0,
        X1,
        X2,
        X3,
        X4,
        X5,
        X6,
        X7,
        X8,
        X9,
        X10,
        X11,
        X12,
        X13,
        X14,
        X15,
        X16,
        X17,
        X18,
        X19,
        X20,
        X21,
        X22,
        X23,
        X24,
        X25,
        X26,
        X27,
        X28,
        X29,
        X30,
        X31,
        REG_END,
        /*
            Other
        */
        INT,
        HEX,
        LABEL_DEF,
        LABEL_DECL,
        COMMA,
        DOT,
        COMMENT,
        LINE_END,
        LBRACK,
        RBRACK,
    };

    inline bool token_kind_is_between(const Token_kind& kind, const Token_kind& kind_start, const Token_kind& kind_end) {
        return (kind > kind_start) && (kind < kind_end);
    }

    inline bool token_is_base_instr(const Token_kind& kind){
        return token_kind_is_between(kind, BASE_INSTR_START, BASE_INSTR_END);
    }

    inline bool token_is_pseudo_instr(const Token_kind& kind){
        return token_kind_is_between(kind, PSEUDO_INSTR_START, PSEUDO_INSTR_END);
    }

    inline bool token_is_reg(const Token_kind& kind){
        return token_kind_is_between(kind, REG_BEGIN, REG_END);
    }

    struct Token{
        std::variant<U32, Instruction_data, std::string> value;
        Token_kind kind;

        bool operator==(const Token& other) const {
            return (value == other.value) && (kind == other.kind);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Token& t){
            stream << t.kind << " ";

            if(token_is_reg(t.kind)){
                stream << "x";
            }

            std::visit([&](const auto& val) {
                stream << val;
            }, t.value);
            
            return stream;
        }
    };

    struct Regex_matcher {

        Regex_matcher(const std::string& p, const Token_kind& k){
            pattern = p;
            kind = k;
        }

        Regex_matcher(const std::string& p, const Token_kind& k, const Instruction_data& d,  bool match_exact = true){
            pattern = p;
            kind = k;
            replacement_value = std::make_optional<Instruction_data>(d);

            // must be an instruction token
            assert(token_is_base_instr(k) || token_is_pseudo_instr(k));

            mod_pattern(match_exact);
        }

        Regex_matcher(const std::string& p, const Token_kind& k, U32 reg_num, bool match_exact = true){
            pattern = p;
            kind = k;
            replacement_value = std::make_optional<U32>(reg_num);
            
            // must be a register token
            assert(token_is_reg(k));

            mod_pattern(match_exact);
        }

        void mod_pattern(const bool& match_exact){
        
            if(match_exact){
                pattern = "^" + pattern + "$";
            }
        }

        std::string pattern;
        Token_kind kind;

        std::optional<std::variant<U32, Instruction_data, std::string>> replacement_value = std::nullopt;
    };
        
    const std::vector<Regex_matcher> TOKEN_RULES = {
        /* R_TYPE */
        Regex_matcher(R"(add)", ADD, Instruction_data(0b0110011, 0x0, 0x00)),
        Regex_matcher(R"(sub)", SUB, Instruction_data(0b0110011, 0x0, 0x20)),
        Regex_matcher(R"(xor)", XOR, Instruction_data(0b0110011, 0x4, 0x00)),
        Regex_matcher(R"(or)", OR, Instruction_data(0b0110011, 0x6, 0x00)),
        Regex_matcher(R"(and)", AND, Instruction_data(0b0110011, 0x7, 0x00)),
        Regex_matcher(R"(sll)", SLL, Instruction_data(0b0110011, 0x1, 0x00)),
        Regex_matcher(R"(srl)", SRL, Instruction_data(0b0110011, 0x5, 0x00)),
        Regex_matcher(R"(sra)", SRA, Instruction_data(0b0110011, 0x5, 0x20)),
        Regex_matcher(R"(slt)", SLT, Instruction_data(0b0110011, 0x2, 0x00)),
        Regex_matcher(R"(sltu)", SLTU, Instruction_data(0b0110011, 0x3, 0x00)),

        /* I_TYPE */
        Regex_matcher(R"(addi)", ADDI, Instruction_data(0b0010011, 0x0, 0x00)),
        Regex_matcher(R"(slli)", SLLI, Instruction_data(0b0010011, 0x1, 0x00)),
        Regex_matcher(R"(slti)", SLTI, Instruction_data(0b0010011, 0x2, 0x00)),
        Regex_matcher(R"(sltiu)", SLTIU, Instruction_data(0b0010011, 0x3, 0x00)),
        Regex_matcher(R"(xori)", XORI, Instruction_data(0b0010011, 0x4, 0x00)),
        Regex_matcher(R"(ori)", ORI, Instruction_data(0b0010011, 0x6, 0x00)),
        Regex_matcher(R"(andi)", ANDI, Instruction_data(0b0010011, 0x7, 0x00)),
        Regex_matcher(R"(srli)", SRLI, Instruction_data(0b0010011, 0x5, 0x00)),
        Regex_matcher(R"(srai)", SRAI, Instruction_data(0b0010011, 0x5, 0x20)),
        Regex_matcher(R"(lb)", LB, Instruction_data(0b0000011, 0x0, 0x00)),
        Regex_matcher(R"(lh)", LH, Instruction_data(0b0000011, 0x1, 0x00)),
        Regex_matcher(R"(lw)", LW, Instruction_data(0b0000011, 0x2, 0x00)),
        Regex_matcher(R"(lbu)", LBU, Instruction_data(0b0000011, 0x4, 0x00)),
        Regex_matcher(R"(lhu)", LHU, Instruction_data(0b0000011, 0x5, 0x00)),
        Regex_matcher(R"(jalr)", JALR, Instruction_data(0b1100111, 0x0, 0x00)),

        /* S_TYPE */
        Regex_matcher(R"(sb)", SB, Instruction_data(0b0100011, 0x0, 0x00)),
        Regex_matcher(R"(sh)", SH, Instruction_data(0b0100011, 0x1, 0x00)),
        Regex_matcher(R"(sw)", SW, Instruction_data(0b0100011, 0x2, 0x00)),

        /* B_TYPE */
        Regex_matcher(R"(beq)", BEQ, Instruction_data(0b1100011, 0x0, 0x00)),
        Regex_matcher(R"(bne)", BNE, Instruction_data(0b1100011, 0x1, 0x00)),
        Regex_matcher(R"(blt)", BLT, Instruction_data(0b1100011, 0x4, 0x00)),
        Regex_matcher(R"(bge)", BGE, Instruction_data(0b1100011, 0x5, 0x00)),
        Regex_matcher(R"(bltu)", BLTU, Instruction_data(0b1100011, 0x6, 0x00)),
        Regex_matcher(R"(bgeu)", BGEU, Instruction_data(0b1100011, 0x7, 0x00)),

        /* U_TYPE */
        Regex_matcher(R"(lui)", LUI, Instruction_data(0b0110111, 0x0, 0x00)),
        Regex_matcher(R"(auipc)", AUIPC, Instruction_data(0b0010111, 0x0, 0x00)),
        
        /* J_TYPE */
        Regex_matcher(R"(jal)", JAL, Instruction_data(0b1101111, 0x0, 0x00)),

        /* P_TYPE */
        Regex_matcher(R"(li)", LI),
        Regex_matcher(R"(la)", LA),
        Regex_matcher(R"(mv)", MV),
        Regex_matcher(R"(not)", NOT),
        Regex_matcher(R"(neg)", NEG),
        Regex_matcher(R"(bgt)", BGT),
        Regex_matcher(R"(ble)", BLE),
        Regex_matcher(R"(bgtu)", BGTU),
        Regex_matcher(R"(bleu)", BLEU),
        Regex_matcher(R"(beqz)", BEQZ),
        Regex_matcher(R"(bnez)", BNEZ),
        Regex_matcher(R"(bgez)", BGEZ),
        Regex_matcher(R"(blez)", BLEZ),
        Regex_matcher(R"(bgtz)", BGTZ),
        Regex_matcher(R"(j)", J),
        Regex_matcher(R"(call)", CALL),
        Regex_matcher(R"(ret)", RET),
        Regex_matcher(R"(nop)", NOP),

        /* Other */
        Regex_matcher(R"(0x[0-9a-fA-F]+)", HEX),

        /* REGISTERS */
        Regex_matcher(R"(x0|zero)", X0, 0),
        Regex_matcher(R"(x1|ra)", X1, 1),
        Regex_matcher(R"(x2|sp)", X2, 2),
        Regex_matcher(R"(x3|gp)", X3, 3),
        Regex_matcher(R"(x4|tp)", X4, 4),
        Regex_matcher(R"(x5|t0)", X5, 5),
        Regex_matcher(R"(x6|t1)", X6, 6),
        Regex_matcher(R"(x7|t2)", X7, 7),
        Regex_matcher(R"(x8|s0|fp)", X8, 8),
        Regex_matcher(R"(x9|s1)", X9, 9),
        Regex_matcher(R"(x10|a0)", X10, 10),
        Regex_matcher(R"(x11|a1)", X11, 11),
        Regex_matcher(R"(x12|a2)", X12, 12),
        Regex_matcher(R"(x13|a3)", X13, 13),
        Regex_matcher(R"(x14|a4)", X14, 14),
        Regex_matcher(R"(x15|a5)", X15, 15),
        Regex_matcher(R"(x16|a6)", X16, 16),
        Regex_matcher(R"(x17|a7)", X17, 17),
        Regex_matcher(R"(x18|s2)", X18, 18),
        Regex_matcher(R"(x19|s3)", X19, 19),
        Regex_matcher(R"(x20|s4)", X20, 20),
        Regex_matcher(R"(x21|s5)", X21, 21),
        Regex_matcher(R"(x22|s6)", X22, 22),
        Regex_matcher(R"(x23|s7)", X23, 23),
        Regex_matcher(R"(x24|s8)", X24, 24),
        Regex_matcher(R"(x25|s9)", X25, 25),
        Regex_matcher(R"(x26|s10)", X26, 26),
        Regex_matcher(R"(x27|s11)", X27, 27),
        Regex_matcher(R"(x28|t3)", X28, 28),
        Regex_matcher(R"(x29|t4)", X29, 29),
        Regex_matcher(R"(x30|t5)", X30, 30),
        Regex_matcher(R"(x31|t6)", X31, 31),

        /* Other */
        Regex_matcher(R"(\-?\d+)", INT),
        Regex_matcher(R"([a-zA-Z_][a-zA-Z0-9_]*:)", LABEL_DEF),
        Regex_matcher(R"([a-zA-Z_][a-zA-Z0-9_]*)", LABEL_DECL),
        Regex_matcher(R"(,)", COMMA),
        Regex_matcher(R"(\.)", DOT),
        Regex_matcher(R"(#)", COMMENT),
        Regex_matcher(R"(\()", LBRACK),
        Regex_matcher(R"(\))", RBRACK),
    };

    const std::string FULL_REGEX = [] {
        std::string regex = "(";

        for (size_t i = 0; i < TOKEN_RULES.size(); i++) {
            regex += TOKEN_RULES[i].pattern;

            if (i + 1 < TOKEN_RULES.size()) regex += "|";
        }
        regex += ")";

        return regex;
    }();

    inline Instruction_data find_instr_data_for(Token_kind kind){

        assert(token_is_base_instr(kind));

        for(size_t i = 0; i < TOKEN_RULES.size(); i++) {
            if(TOKEN_RULES[i].kind == kind) {
                return std::get<Instruction_data>(TOKEN_RULES[i].replacement_value.value());
            }
        }

        return Instruction_data();
    }
            
    class Lexer{
        public:
            Lexer(){}

            Lexer(std::string filename)
                :_filename(filename)
            {
                lex();
            }

            inline bool string_is(const std::string& string, const std::string& pattern){
                bool matches = std::regex_match(string, std::regex(pattern));
                return ((ignore == false) && matches) || (string == "*/") ;
            }

            inline void lex(){
                std::string input, matched_string;
                std::ifstream stream(_filename);
                
                std::regex full_pattern(FULL_REGEX, std::regex::icase);

                std::sregex_iterator end;

                while(std::getline(stream, input)){

                    if(input == "") continue;
                    
                    std::sregex_iterator begin(input.begin(), input.end(), full_pattern);

                    for(std::sregex_iterator i = begin; (i != end); ++i){
                        std::smatch match = *i;
                        matched_string = match.str();

                        if(string_is(matched_string, R"(#)")){
                            break;

                        } else if(string_is(matched_string, R"(/\*)")){
                            ignore = true;
                            break;
                        
                        } else if (string_is(matched_string, R"(\*/)")){
                            ignore = false;
                            break;

                        } else {
                        
                            for(const Regex_matcher& tr : TOKEN_RULES){
                                if(string_is(matched_string, tr.pattern)){
                                    tokens.push_back(Token{tr.replacement_value.value_or(matched_string), tr.kind});                        

                                    break;
                                }
                            }
                        }
                    }

                    tokens.push_back(Token{.value = "LINE_END", .kind = LINE_END});
                }

                tokens.push_back(Token{.value = "", .kind = _EOF});        
            }

            inline void print_tokens() const {

                for(size_t i = 0; i < tokens.size(); ++i){
                    std::cout << tokens[i] << std::endl;
                }
            }

            inline std::vector<Token> get_tokens(){
                return tokens;
            }

        private:
            std::vector<Token> tokens;
            std::string _filename = ""; 
            bool ignore = false;
            
    };

}


