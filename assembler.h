#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "parser.h"
#include "defs.h"
#include <iomanip>
class assembler{
    public:
        assembler(){}
        void SetCurrentPath(const fs::path& path){current_path = path;}

        void setlabels(){
            std::string current_instr;

            current_instr = getcurrentinstruction();
            instruction_tokens = GetTokens(current_instr, std::regex(FULL_REGEX));

            if(peek() == ":"){
                // the first token is a label
                token_pointer++;
                
                symbol_table[previoustoken()] = (hasmoretokens()) ? mem_address : mem_address + 1;
            } else {
                if(getcurrenttoken() != "."){mem_address++;}
            }

            instruction_pointer += 1;
            token_pointer = 0;
        }

        void assemble(){
            parser.SetPathToFile(current_path);
            parser.Parse();
            instructions = parser.GetInstrs();

            std::string output_path = current_path.replace_extension(".txt"); 
            std::ofstream outfile(output_path);

            // do a first pass to sort out labels
            while(hasmoreinstructions()){
                setlabels();
            }

            // do second pass to do actual assembly
            instruction_pointer = 0;  
            token_pointer = 0;
            mem_address = 0;

            // std::cout << instructions[0] << std::endl;

            instruction_tokens = GetTokens(instructions[0], std::regex(FULL_REGEX));
            std::string directive;

            while(getcurrenttoken() == "."){
                processdirective();
                directive = getcurrentinstruction();
                instruction_tokens = GetTokens(directive, std::regex(FULL_REGEX));
            }

            while(hasmoreinstructions()){
                process_instructions(outfile);
            }

            instructions = {};
            current_instr_mc = 0;
            instruction_pointer = 0;

            std::cout << "Machine code in " << output_path << std::endl;

        }

        void processdirective(){
            // todo
            token_pointer++;  // now pointing at directive command

            std::string directive_command = getcurrenttoken();

            if(directive_command == "equ"){
                token_pointer++;
                auto binding = getcurrenttoken();
                token_pointer++;
                processsyntax(",");
                auto imm = stringtoint(getcurrenttoken());
                imm_bindings[binding] = imm;
            } else if(directive_command == "text") {
                std::cout << "Assembly after this command" << std::endl;
            } else if(directive_command == "global" || directive_command == "globl"){
                std::cout << "Set label to global scope" << std::endl;
            } else {
                std::cout << "Invalid directive " << directive_command << std::endl;
                exit(0); 
            }

            std::cout << directive_command << std::endl;

            instruction_pointer++;
            token_pointer = 0;
        }
            
        void process_instructions(std::ofstream& outfile){
            // instructions is either label or RISCV instruction
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
                    processinstruction(outfile);
                } 
    
            } else {
                // we have a RISCV instruction
                processinstruction(outfile);
            }

            instruction_pointer += 1;
            token_pointer = 0;

            // machine_code.push_back(current_instr_mc);
            current_instr_mc = 0;
        }

        void process_J_type(const tableRow& opcodes){
            uint32_t imm;

            processregister(0);
            processsyntax(",");

            if(isvalidNum(getcurrenttoken())){
                imm = stringtoint(getcurrenttoken()) - mem_address - 1;
            } else {
                imm = symbol_table[getcurrenttoken()] - mem_address - 1;
            }

            imm *= 4;

            processimmediate(opcodes, imm);
        }

        void process_U_type(const tableRow& opcodes){
            uint32_t imm;

            processregister(0);
            processsyntax(",");

            checkimm(getcurrenttoken(), imm);
            
            processimmediate(opcodes, imm);
        }        

        void process_S_type(const tableRow& opcodes){
            uint32_t imm;

            processregister(2);
            processsyntax(",");

            checkimm(getcurrenttoken(), imm);
            processimmediate(opcodes, imm);

            processsyntax("(");
            processregister(1);
            processsyntax(")");
        }

        void process_R_type(){
            // we know the next register as rd
            processregister(0);
            processsyntax(",");
            processregister(1);
            processsyntax(",");
            processregister(2);
        }

        void process_I_type(const tableRow& opcodes){
            uint32_t imm;

            // we know the next register is rd
            processregister(0);
            processsyntax(",");

            if(opcodes.op == 3){
                // next token is an immediate
                checkimm(getcurrenttoken(), imm);

                processimmediate(opcodes, imm);
                processsyntax("(");
                processregister(1);
                processsyntax(")");
                
            } else if (opcodes.op == 19 || opcodes.op == 103){
                // next token is rs1
                processregister(1);
                processsyntax(",");

                checkimm(getcurrenttoken(),imm);

                processimmediate(opcodes, imm);
                
            } else {
                std::cout << "What? Opcode should be 3, 19 or 103 for I-type. This is " << opcodes.op << ".\n";
                std::cout << getcurrentinstruction() << std::endl;
                exit(0);
            }
        }

        void checkimm(const std::string& immstr, uint32_t& immint){
            if(isvalidNum(immstr)){
                immint = stringtoint(getcurrenttoken());
            } else if(imm_bindings.find(getcurrenttoken()) != imm_bindings.end()){
                immint = imm_bindings[getcurrenttoken()];
            } else {
                std::cout << "Invalid immediate " << immstr << std::endl;
                exit(0);
            }
        }

        void process_B_type(tableRow& opcodes){
            // we know the next register is rs1
            uint32_t imm = 0;

            processregister(1);
            processsyntax(",");
            processregister(2);
            processsyntax(",");

            if(isvalidNum(getcurrenttoken())){
                imm = stringtoint(getcurrenttoken()) - mem_address - 1;
            } else {
                imm = symbol_table[getcurrenttoken()] - mem_address - 1;
            }

            imm *= 4;

            processimmediate(opcodes, imm);
        }

        void processinstruction(std::ofstream& outfile){
            // tableRow opcodes;

            tableRow opcodes;

            int instructiontype = processopcode(getcurrenttoken(), opcodes);
            // rd -> 0, rs1 -> 1, rs2 -> 2
            switch(instructiontype){
                case 0: process_R_type(); break;
                case 1: process_I_type(opcodes); break;
                case 2: process_S_type(opcodes); break;
                case 3: process_B_type(opcodes); break;
                case 4: process_U_type(opcodes); break;
                case 5: process_J_type(opcodes); break; 
            }

            outfile << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;
            // std::cout << numtobin(current_instr_mc) << std::endl;
            mem_address ++;
        }

        void processimmediate(const tableRow& opcodes, uint32_t immediate){
        
            switch(opcodes.op){
                case 19:
                    if(opcodes.funct3 == 1 || opcodes.funct3 == 5){
                        // immediate is unsigned 5 bit immediate
                        current_instr_mc |= ((immediate & 31) << 20);
                    } else {
                        current_instr_mc |= ((immediate & 4095) << 20);
                    }
                    break;

                case 3: case 103:
                    current_instr_mc |= ((immediate & 4095) << 20);
                    break;

                case 35:
                    current_instr_mc |= ((immediate & 31) << 7);
                    current_instr_mc |= ((immediate & 4064) << 20);
                    break;

                case 99:
                    current_instr_mc |= ((immediate & 4096) << 19);
                    current_instr_mc |= ((immediate & 2016) << 20);
                    current_instr_mc |= ((immediate & 2048) >> 4);
                    current_instr_mc |= ((immediate & 30) << 7);
                    break;

                case 23: case 55:
                    current_instr_mc |= (immediate & 4294963200);
                    break;

                case 111:
                    current_instr_mc |= ((immediate & 1048576) << 11);
                    current_instr_mc |= ((immediate & 2046) << 20);
                    current_instr_mc |= (immediate & 1044480);
                    current_instr_mc |= ((immediate & 2048) << 9);
                    break;
                
                default:
                    std::cout << "Invalid opcode value " << numtobin(opcodes.op) << std::endl;
                    exit(0);
            }

            token_pointer++;
        }

        void processregister(const int& regtype){
            std::string reg = getcurrenttoken();
            uint32_t regnum;
            
            if(checkregister(reg)){
                // valid register
                uint32_t regnum_prime = codes.getregnum(reg);
                regnum = (regnum_prime == 32) ? codes.getspecialregnum(reg) : regnum_prime;

                switch(regtype){
                    case 0 : current_instr_mc |= (regnum << 7); break; // rd
                    case 1 : current_instr_mc |= (regnum << 15); break; // rs1
                    case 2 : current_instr_mc |= (regnum << 20); break; // rs2
                }

            } else {
                // std::cout << std::regex_match("s11", std::regex(REGISTERS)) << std::endl;
                std::cout << "Invalid register " << reg <<  std::endl;
                exit(0);
            }

            token_pointer++;

        }

        int processopcode(std::string opcode, tableRow& r){
            int instructiontype = checkopcode(opcode);

            if(instructiontype == -1){
                std::cout << "Invalid opcode " << opcode << std::endl;
                exit(0);
            } else {
                // process the instruction
                r = codes.getcontrolbits(opcode);
                // std::cout << (int)r.op << (int)r.funct3 << (int)r.funct7 << std::endl; 

                current_instr_mc |= r.op;
                current_instr_mc |= (r.funct3 << 12);
                current_instr_mc |= (r.funct7 << 25);
            }

            token_pointer ++;

            return instructiontype;
        }

        void processsyntax(const std::string& expected_token){
            if(getcurrenttoken() == expected_token){
                token_pointer ++;
            } else {
                std::cout << "Expected " << expected_token << " after " << previoustoken() << " instead of " << getcurrenttoken() << std::endl;
                exit(0);
            }
        }

        bool hasmoreinstructions(){
            return instruction_pointer != (int)instructions.size();
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
            std::vector<std::regex> registers = {t0to2, s0to1, a0to7, s2to11, t3to6, x_regnum, SPECIAL_REGS};

            for(auto pattern : registers){
                if(std::regex_match(reg, pattern)){
                    return true;
                }
            }
            return false;
        }


    private:
        fs::path current_path;

        std::vector<std::string> instruction_tokens;
        int token_pointer = 0;
        std::vector<std::string> instructions;
        int instruction_pointer = 0;
        int mem_address = 0;

        std::vector<uint32_t> machine_code;

        uint32_t current_instr_mc = 0;

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

        // symbol table and equ directive table
        std::unordered_map<std::string, int> symbol_table;
        std::unordered_map<std::string, int> imm_bindings;

        // codes
        machine_codes codes;
};



#endif