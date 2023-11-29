#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "parser.h"
#include "defs.h"

class assembler{
    public:
        assembler(){}
        void SetCurrentPath(const std::string& path){current_path = path;}

        void setlabels(){
            std::string current_instr;

            current_instr = getcurrentinstruction();
            instruction_tokens = GetTokens(current_instr, std::regex(FULL_REGEX));

            if(peek() == ":"){
                // the first token is a label
                symbol_table[getcurrenttoken()] = instruction_pointer;
            }

            instruction_pointer += 1;
            token_pointer = 0;

        }

        void assemble(){
            parser.SetPathToFile(current_path);
            parser.Parse();
            instructions = parser.GetInstrs();

            // do a first pass to sort out labels
            while(hasmoreinstructions()){
                setlabels();
            }
            setlabels();

            // do second pass to do actual assembly
            instruction_pointer = 0;   // make sure to reset instruction pointer to 0
            while(hasmoreinstructions()){
                process_statements();
            }
            process_statements();

        }

        void process_statements(){
            // statement is either label or RISCV instruction
            std::string current_instr;
            std::string opcode;

            current_instr = getcurrentinstruction();
            instruction_tokens = GetTokens(current_instr, std::regex(FULL_REGEX));

            if(peek() == ":"){
                // check for RISCV instruction on same line as label
                token_pointer ++;
                opcode = peek();
                
                if(opcode != "notokens"){
                    token_pointer ++;  // token pointer now points to the opcode
                    processinstruction();
                } 
    
            } else {
                // we have a RISCV instruction
                processinstruction();
            }

            instruction_pointer += 1;
            token_pointer = 0;

            machine_code.push_back(current_instr_mc);
            current_instr_mc = 0;
        }

        void processinstruction(){
            int instructiontype = processopcode();
            int regtype;

            /**
             * rd -> 0
             * rs1 -> 1
             * rs2 -> 2
            */

            switch(instructiontype){
                case 0 | 1 | 4 | 5 : regtype = 0;  // r, i, u, j
                case 2 : regtype = 2;      // s
                case 3 : regtype = 1;      // b
            }


            processregister(instructiontype, regtype);

        }

        void processregister(const int& instructiontype, const int& regtype){
            std::string reg = getcurrenttoken();
            uint32_t regnum;

            if(checkregister(reg)){
                // valid register
                uint32_t regnum_prime = codes.getregnum(reg);
                regnum = (regnum_prime == 32) ? codes.getspecialregnum(reg) : regnum_prime;
                
                switch(regtype){
                    case 0 : current_instr_mc |= (regnum << 7);  // rd
                    case 1 : current_instr_mc |= (regnum << 15); // rs1
                    case 2 : current_instr_mc |= (regnum << 20); // rs2
                }

                std::cout << std::hex << current_instr_mc << std::endl;

            } else {
                std::cout << "Invalid register" << std::endl;
            }

            token_pointer++;

        }

        int processopcode(){
            std::string opcode = getcurrenttoken();
            int instructiontype = checkopcode(opcode);

            if(instructiontype == -1){
                std::cout << "Invalid opcode " << opcode << std::endl;
            } else {
                // process the instruction
                tableRow r = codes.getcontrolbits(opcode);
                // std::cout << (int)r.op << (int)r.funct3 << (int)r.funct7 << std::endl; 

                current_instr_mc |= r.op;
                current_instr_mc |= (r.funct3 << 12);
                current_instr_mc |= (r.funct7 << 25);
            }

            token_pointer ++;

            return instructiontype;
        }

        bool hasmoreinstructions(){
            return instruction_pointer != (int)instructions.size()-1;
        }

        std::string getcurrentinstruction(){
            return instructions[instruction_pointer];
        }

        bool hasmoretokens(){
            return token_pointer != (int)instruction_tokens.size()-1;
        }

        std::string getcurrenttoken(){
            return instruction_tokens[token_pointer];
        }

        std::string previoustoken(){
            return instruction_tokens[token_pointer-1];
        }

        std::string peek(){
            if(hasmoretokens()){
                return instruction_tokens[token_pointer+1];
            } else {
                return "notokens";
            }
        }

        int checkopcode(const std::string& opcode){
            std::vector<std::string> opcodes = {R_TYPE_OPCODES, I_TYPE_OPCODES, S_TYPE_OPCODES, B_TYPE_OPCODES, U_TYPE_OPCODES, J_TYPE_OPCODES};

            for(auto pattern : opcodes){
                if(std::regex_match(opcode, std::regex(pattern))){
                    return opcode_types[pattern];
                }
            }
            return -1;
        }

        bool checkregister(const std::string& reg){
            return std::regex_match(reg, std::regex(REGISTERS));
        }


    private:
        std::string current_path;

        std::vector<std::string> instruction_tokens;
        int token_pointer = 0;
        std::vector<std::string> instructions;
        int instruction_pointer = 0;

        std::vector<uint32_t> machine_code;
        uint32_t current_instr_mc;

        Parser parser;  

        // opcode types
        std::unordered_map<std::string, int> opcode_types = {
            {R_TYPE_OPCODES, 0},    // r
            {I_TYPE_OPCODES, 1},    // i
            {S_TYPE_OPCODES, 2},    // s
            {B_TYPE_OPCODES, 3},    // b
            {U_TYPE_OPCODES, 4},    // u
            {J_TYPE_OPCODES, 5}     // j
        };

        // symbol table
        std::unordered_map<std::string, int> symbol_table;

        // codes
        machine_codes codes;
};



#endif