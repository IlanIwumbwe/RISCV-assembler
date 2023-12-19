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

            processoffset(opcodes, imm);
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

        void process_B_type(const tableRow& opcodes){
            // we know the next register is rs1
            uint32_t imm = 0;

            processregister(1);
            processsyntax(",");
            processregister(2);
            processsyntax(",");

            processoffset(opcodes, imm);
        }

        /// Process offsets for instructions that need to jump addresses
        void processoffset(const tableRow& opcodes, uint32_t& imm){
            if(isvalidNum(getcurrenttoken())){
                imm = stringtoint(getcurrenttoken()) - mem_address - 1;
            } else {
                imm = symbol_table[getcurrenttoken()] - mem_address - 1;
            }

            imm *= 4;

            processimmediate(opcodes, imm);
        }

        uint32_t getdestreg(){
            return (current_instr_mc & 3968) >> 7;
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

            if(instructiontype == 6){
                int baseinstructions = 0;

                baseinstructions = processpsuedoinstruction(opcodes, outfile);

                if(baseinstructions > 1){
                    // go through all labels below this pseudo instruction and change the address they point to 
                    for(auto& p : symbol_table){
                        if(p.second > mem_address){p.second += baseinstructions - 1;}
                    }
                }

                mem_address += baseinstructions;

            } else {
                outfile << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;
                mem_address ++;
            }
        }

        /// Process pseudoinstruction and return number of base instructions
        int processpsuedoinstruction(tableRow& opcodes, std::ofstream& outfile){
            auto p_opcode = previoustoken();
            uint32_t dest_reg = 0, imm = 0;

            uint32_t ready_base_instruction = 0;

            if(p_opcode == "li" || p_opcode == "la"){
                processregister(0);

                dest_reg = getdestreg();

                processsyntax(",");

                checkimm(getcurrenttoken(),imm);
                processimmediate(opcodes, imm);

                if(imm > 4095 && imm & 4095){
                    // need extra upper instruction for imm > 12 bits
                    processregister(1, dest_reg);

                    //std::cout << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;

                    ready_base_instruction = current_instr_mc;
                    current_instr_mc = 0;
        
                    if(p_opcode == "li"){processopcode("lui", opcodes);} else {processopcode("auipc", opcodes);}

                    processregister(0, dest_reg);
                    
                    // check whether we need to add 1 to upper instruction immediate for lui or auipc instruction
                    if(imm & 2048){processimmediate(opcodes, imm+1);} else {processimmediate(opcodes, imm);}

                    // std::cout << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;

                    outfile << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;
                    outfile << std::setfill('0') << std::setw(8) << std::hex << ready_base_instruction << std::endl;

                    return 2;
                }

            } else if (p_opcode == "mv" || p_opcode == "not" || p_opcode == "neg"){
                processregister(0);
                processsyntax(",");                                                 
                processregister(1);

                if(p_opcode != "neg"){
                    imm = (p_opcode == "mv") ? 0 : -1;
                    processimmediate(opcodes, imm);
                }            

            } else if(p_opcode == "bgt" || p_opcode == "ble" || p_opcode == "bgtu" || p_opcode == "bleu" || p_opcode == "beqz" ||
                p_opcode == "bnez" || p_opcode == "bgez" || p_opcode == "blez" || p_opcode == "bgez"){

                if(p_opcode == "bgt" || p_opcode == "ble" || p_opcode == "bgtu" || p_opcode == "bleu"){
                    processregister(2);
                    processsyntax(",");
                    processregister(1);
                } else if(p_opcode == "beqz" || p_opcode == "bnez" || p_opcode == "bgez"){
                    processregister(2);
                } else {
                    processregister(1);
                }

                processsyntax(",");

                processoffset(opcodes, imm);
            } else if(p_opcode == "j"){
                processoffset(opcodes, imm);

            } else if(p_opcode == "call"){
                processoffset(opcodes, imm);

                processregister(0, 1);
                processregister(1, 1);

                if(imm > 4095){
                    ready_base_instruction = current_instr_mc;
                    current_instr_mc = 0;

                    processopcode("auipc", opcodes);   
                    processregister(0, 1);
                    processimmediate(opcodes, imm);

                    outfile << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;
                    outfile << std::setfill('0') << std::setw(8) << std::hex << ready_base_instruction << std::endl;

                    return 2;
                }

            } else if(p_opcode == "ret"){
                processregister(1, 1);
                
            } else {
                std::cout << "nop instruction should be the only one that reaches this point" << std::endl;
                // NOP, do nothing
            }
    
            outfile << std::setfill('0') << std::setw(8) << std::hex << current_instr_mc << std::endl;
            return 1;
        }

        void processimmediate(const tableRow& opcodes, uint32_t immediate){
        
            switch(opcodes.op){
                case 19:
                    if(opcodes.funct3 == 1 || opcodes.funct3 == 5){
                        // immediate is unsigned 5 bit immediate (shift instructions)
                        current_instr_mc |= ((immediate & 31) << 20);
                        if(immediate > 4095){
                            std::cout << "Warning: The immediate passed for the shift instruction " << getcurrentinstruction() << " is more than 5 bits wide. ";
                            std::cout << "Only the least significant 5 bits will be considered." << std::endl;
                        }
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

        /// Given the expected register type, compare with the current token to check if that's what we have
        /// Show error and exit if not
        /// rd = 0, rs1 = 1, rs2 = 2
        void processregister(const int& regtype, const uint32_t rnum = 32){
            uint32_t regnum = 0;
            
            if(rnum >= 32){
                std::string reg = getcurrenttoken();

                if(checkregister(reg)){
                    // valid register
                    uint32_t regnum_prime = codes.getregnum(reg);
                    regnum = (regnum_prime == 32) ? codes.getspecialregnum(reg) : regnum_prime;

                } else {
                    // std::cout << std::regex_match("s11", std::regex(REGISTERS)) << std::endl;
                    std::cout << "Invalid register " << reg <<  std::endl;
                    exit(0);
                }

            } else {
                // regnum has been passed as a function argument already, no need to look for it using current token
                regnum = rnum;
            }

            switch(regtype){
                case 0 : current_instr_mc |= (regnum << 7); break; // rd
                case 1 : current_instr_mc |= (regnum << 15); break; // rs1
                case 2 : current_instr_mc |= (regnum << 20); break; // rs2
            }

            token_pointer++;

        }


        /// Given an assembly opcode, fill in the machine code for it. For pseudo instructions, fill in opcode of base instruction. 
        /// If there are more than one psuedo instruction, fill in opcode for the first base instruction
        int processopcode(std::string opcode, tableRow& r){
            int instructiontype = checkopcode(opcode);

            if(instructiontype == -1){
                std::cout << "Invalid opcode " << opcode << std::endl;
                exit(0);
            } else {
                // process the instruction
                r = codes.getcontrolbits(opcode);

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
            std::vector<std::string> opcodes = {R_TYPE_OPCODES, I_TYPE_OPCODES, S_TYPE_OPCODES, B_TYPE_OPCODES, U_TYPE_OPCODES, J_TYPE_OPCODES, P_OPCODES};

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
            {J_TYPE_OPCODES, 5},    // j
            {P_OPCODES, 6},         // psuedoinstruction
        };

        // symbol table and equ directive table
        std::unordered_map<std::string, int> symbol_table;
        std::unordered_map<std::string, int> imm_bindings;

        // codes
        machine_codes codes;
};



#endif