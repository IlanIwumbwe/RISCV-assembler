#pragma once

#include "lex.h"

namespace Assembler {

    class Assembler {

        public:
            Assembler(std::vector<Token> _tokens, fs::path path) :
                tokens(_tokens),
                stream(path.replace_extension(".txt"))
            {
                num_tokens = tokens.size();
            }
            
            void consume(int n){
                prev_token = tokens[token_pointer];

                token_pointer += n;
                        
                if(token_pointer == num_tokens){
                    ERROR("Out of tokens! Token pointer: " + std::to_string(token_pointer) + " num_tokens: " + std::to_string(num_tokens));
                } else {
                    curr_token = tokens[token_pointer];
                }
            }

            void consume(const Token_kind kind){

                if(curr_token.kind == kind){
                    consume(1);
                } else {
                    std::cout << "Current token " << std::endl;
                    std::cout << curr_token << std::endl;
                    PANIC("Expected kind: " + std::to_string(kind) + " Current token not matched");
                }
            }

            void peek(int n = 1){
                if((token_pointer + n) == num_tokens){
                    ERROR("Cannot peek!");
                } else {
                    next_token = tokens[token_pointer + n];
                }
            }

            void set_labels(){

                while(curr_token.kind != _EOF){

                    std::cout << YELLOW(std::to_string(pc) + ": ") << curr_token << std::endl;

                    if((curr_token.kind == LINE_END) && (prev_token.kind != LABEL_DEF)){
                        pc += 1;

                    } else if (curr_token.kind == LABEL_DEF) {
                        // label definition, set in symbol table
                        symbol_table[std::get<std::string>(curr_token.value)] = pc;
                    }

                    consume(1);
                }
            }

            void reset(){
                pc = 0;
                token_pointer = 0;
                consume(0);
            }

            void emit_binary(){
                if(current_instr_binary){ 
                    to_file(stream, current_instr_binary);
                    current_instr_binary = 0UL;
                }
            }

            void set_register(Register_port port, U32 reg_num){
                switch(port){
                    case RD : current_instr_binary |= (reg_num << 7); break; 
                    case RS1 : current_instr_binary |= (reg_num << 15); break; 
                    case RS2 : current_instr_binary |= (reg_num << 20); break; 
                }
            }

            void consume_reg(Register_port port){
                U32 reg_num;

                assert(token_is_reg(curr_token.kind));
                reg_num = std::get<U32>(curr_token.value);

                set_register(port, reg_num);

                consume(1);
            }

            U64 get_imm(bool calc_pc_offset = false, bool prefer_label = false){

                U64 addr = 0;

                if((curr_token.kind == HEX || curr_token.kind == INT) && !prefer_label){
                    // address to jump to given, calculate offset directly
                    const char* addr_str = std::get<std::string>(curr_token.value).c_str();
                    char* endptr;

                    addr = std::strtoul(addr_str, &endptr, 0);
                
                } else if (curr_token.kind == LABEL_DECL){
                    addr = symbol_table[std::get<std::string>(curr_token.value) + ":"];
                
                } else {
                    std::cout << "Current token: " << std::endl << curr_token << std::endl;
                    if(prefer_label){ 
                        PANIC("Current token must be a label!");
                    } else {
                        PANIC("Only HEX, INT, LABEL_DECL can give imm!");
                    }
                }

                return (calc_pc_offset) ? (addr - pc) * 4 : addr;
            }

            int process_r_type_instr(const Instruction_data& instr_data){
                
                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);
                place_bits(current_instr_binary, instr_data.get_funct7(), 25, 7);

                if(instr_data.from_pseudo_instr()){
                    std::cout << "Adding R-type from pseudo instr" << std::endl;

                    Pseudo_instruction_data pseudo_instr_data = instr_data.get_psi_data();

                    set_register(RD, pseudo_instr_data.rd);
                    set_register(RS1, pseudo_instr_data.rs1);
                    set_register(RS2, pseudo_instr_data.rs2);
                
                } else {

                    consume(1);

                    consume_reg(RD);

                    consume(COMMA);

                    consume_reg(RS1);

                    consume(COMMA);

                    consume_reg(RS2);
                } 

                return 1;
            }

            int process_i_type_instr(const Instruction_data& instr_data){

                U32 opcode = instr_data.get_opcode();
                U64 imm;
                
                current_instr_binary |= opcode;
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);

                if(instr_data.from_pseudo_instr()){
                    std::cout << "Adding I-type from pseudo instr" << std::endl;

                    Pseudo_instruction_data pseudo_instr_data = instr_data.get_psi_data();

                    set_register(RD, pseudo_instr_data.rd);
                    imm = pseudo_instr_data.imm;
                    set_register(RS1, pseudo_instr_data.rs1);
                
                } else {

                    consume(1);

                    consume_reg(RD);

                    consume(COMMA);

                    if(opcode == 0b0000011){
                        
                        imm = get_imm();

                        consume(1);

                        consume(LBRACK);

                        consume_reg(RS1);

                        consume(RBRACK);
                        
                    } else if (opcode == 0b0010011 || opcode == 0b1100111){

                        consume_reg(RS1);
                        
                        consume(COMMA);
                        
                        imm = get_imm(); 

                        consume(1);
                    }

                }

                if(instr_data.get_funct3() == 1 || instr_data.get_funct3() == 5){
                    // imm is unsigned 5 bit
                    place_bits(current_instr_binary, get_bits(imm, 4, 0), 20, 12);

                    if(imm > 4095){
                        WARNING("The immediate passed for the shift instruction at " + std::to_string(pc) + " is more than 5 bits wide, only the least significant 5 bits will be considered");
                    }

                } else {
                    place_bits(current_instr_binary, get_bits(imm, 11, 0), 20, 12);
                }

                return 1;
            }

            int process_s_type_instr(const Instruction_data& instr_data){

                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);
                consume(1);

                consume_reg(RS2);

                consume(COMMA);

                U64 imm = get_imm();

                place_bits(current_instr_binary, get_bits(imm, 4, 0), 7, 5);
                place_bits(current_instr_binary, get_bits(imm, 11, 5), 25, 7);
                consume(1);

                consume(LBRACK);

                consume_reg(RS1);
                
                consume(RBRACK);

                return 1;
            }

            int process_b_type_instr(const Instruction_data& instr_data){
                
                U64 imm;

                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);

                if(instr_data.from_pseudo_instr()){
                    Pseudo_instruction_data pseudo_instr_data = instr_data.get_psi_data();
                    
                    set_register(RS1, pseudo_instr_data.rs1);
                    set_register(RS2, pseudo_instr_data.rs2);
                    imm = pseudo_instr_data.imm;
                
                } else {

                    consume(1);

                    consume_reg(RS1);
                    
                    consume(COMMA);

                    consume_reg(RS2);
                    
                    consume(COMMA);

                    imm = get_imm(true, true);

                    consume(1);
                }

                place_bits(current_instr_binary, get_bits(imm, 11, 11), 7, 1);
                place_bits(current_instr_binary, get_bits(imm, 4, 1), 8, 4);
                place_bits(current_instr_binary, get_bits(imm, 10, 5), 25, 6);
                place_bits(current_instr_binary, get_bits(imm, 12, 12), 31, 1);

                return 1;
            }

            int process_u_type_instr(const Instruction_data& instr_data){

                U64 imm;

                current_instr_binary |= instr_data.get_opcode();

                if(instr_data.from_pseudo_instr()){

                    std::cout << "Adding LUI because of pseudo " << std::endl;
                    
                    Pseudo_instruction_data pseudo_instr_data = instr_data.get_psi_data(); 

                    set_register(RD, pseudo_instr_data.rd);

                    std::cout << "U type IMM: " << HEX(pseudo_instr_data.imm) << std::endl;

                    imm = pseudo_instr_data.imm;

                } else {

                    consume(1);

                    consume_reg(RD);
                    
                    consume(COMMA);

                    imm = get_imm();

                    consume(1);

                    if(FITS_IN_20_BITS(imm)){
                        place_bits(current_instr_binary, get_bits(imm, 19, 0), 12, 20);
                        return 1;

                    } else {
                        std::cout << "U_type imm " << pc << " : " << HEX(imm) << std::endl;
                        PANIC("Imm does not fit in 20 bits!"); 
                    }
                }

                place_bits(current_instr_binary, get_bits(imm, 31, 12), 12, 20);

                return 1;
            }

            int process_j_type_instr(const Instruction_data& instr_data){

                current_instr_binary |= instr_data.get_opcode();

                consume(1);
                
                consume_reg(RD);
                
                consume(COMMA);

                U64 imm = get_imm(true);

                consume(1);

                // imm[20|10:1|11|19:12]
                place_bits(current_instr_binary, get_bits(imm, 19, 12), 12, 8);
                place_bits(current_instr_binary, get_bits(imm, 11, 11), 20, 1);
                place_bits(current_instr_binary, get_bits(imm, 10, 1), 21, 10);
                place_bits(current_instr_binary, get_bits(imm, 20, 20), 31, 1);

                return 1;
            }

            int process_p_instr(){
                /*
                    Pseudo instructions? I hardly know her
                */

                Pseudo_instruction_data pseudo_instr_data {0};
                Instruction_data instr_data;
                U64 imm;
                Token instr_token = curr_token;
                Token imm_token;

                consume(1);

                if((instr_token.kind == LI) | (instr_token.kind == LA)){

                    pseudo_instr_data.rd = curr_token.get_reg_num();

                    consume(1);

                    consume(COMMA);

                    imm_token = curr_token;
                    imm = get_imm();

                    consume(1);

                    if(imm > 4095){
                        /*
                            check whether we need to add 1 to upper instruction immediate for U-type instr
                            if the 12th bit is set, we need to add 1 to the immediate
                        */
                        pseudo_instr_data.imm = imm + ((imm & 2048) != 0ULL);

                        // rs1 of the U-type is rd in this instr
                        pseudo_instr_data.rs1 = pseudo_instr_data.rd;

                        // emit U-type instr
                        instr_data = find_instr_data_for((instr_token.kind == LA) && (imm_token.kind == LABEL_DECL) ? AUIPC : LUI);
                        instr_data.set_pseudo_instr_data(pseudo_instr_data);

                        process_u_type_instr(instr_data);
                        emit_binary();

                        /*
                            if lower 12 bits of imm are set, emit addi
                        */
                        if(imm & 2047){
                            pseudo_instr_data.imm = imm; // set back to original in case of +1 for U-type
                            instr_data = find_instr_data_for(ADDI);
                            instr_data.set_pseudo_instr_data(pseudo_instr_data);

                            process_i_type_instr(instr_data);
                            emit_binary();

                            return 2;
                        }
    
                    } else {

                        /*
                            result fits in 12 bits
                        */                    
                        instr_data = find_instr_data_for(ADDI);
                        instr_data.set_pseudo_instr_data(pseudo_instr_data);

                        process_i_type_instr(instr_data);
                        emit_binary();

                    }

                    return 1;
                
                } else if ((instr_token.kind == MV) || (instr_token.kind == NOT)){

                    pseudo_instr_data.rd = curr_token.get_reg_num();

                    consume(1);

                    consume(COMMA);

                    pseudo_instr_data.rs1 = curr_token.get_reg_num();

                    consume(1);

                    if(instr_token.kind == MV){
                        instr_data = find_instr_data_for(ADDI);

                    } else {
                        instr_data = find_instr_data_for(XORI);
                        pseudo_instr_data.imm = -1;
                    }

                    instr_data.set_pseudo_instr_data(pseudo_instr_data);
                    
                    process_i_type_instr(instr_data);
                    emit_binary();
                
                } else if (instr_token.kind == NEG){

                    pseudo_instr_data.rd = curr_token.get_reg_num();

                    consume(1);

                    consume(COMMA);

                    pseudo_instr_data.rs2 = curr_token.get_reg_num();

                    consume(1);

                    instr_data = find_instr_data_for(SUB);
                    instr_data.set_pseudo_instr_data(pseudo_instr_data);
                    
                    process_r_type_instr(instr_data);
                    emit_binary();
                
                } else if ((instr_token.kind == BGT) || (instr_token.kind == BLE) || (instr_token.kind == BGTU) || (instr_token.kind == BLEU)){
                    // TODO: fix these pseudos

                    pseudo_instr_data.rs2 = curr_token.get_reg_num();

                    consume(1);

                    consume(COMMA);

                    pseudo_instr_data.rs1 = curr_token.get_reg_num();

                    consume(1);

                    consume(COMMA);

                    pseudo_instr_data.imm = get_imm();

                    consume(1);

                    instr_data = find_instr_data_for(pseudo_to_base[instr_token.kind]);
                    instr_data.set_pseudo_instr_data(pseudo_instr_data);

                    process_b_type_instr(instr_data);
                    emit_binary();
                }

                return 1;
            }

            void process(){

                if(curr_token.kind == _EOF){
                    return;
                }

                if(curr_token.kind == LABEL_DEF){
                    // handled in `set_labels`
                    consume(1);

                } else if(curr_token.kind == LINE_END){ 

                    if(prev_token.kind != LABEL_DEF){
                        emit_binary();      
                    }

                    consume(1);
                
                } else if(token_kind_is_between(curr_token.kind, R_TYPE_START, R_TYPE_END)){ 
                    pc += process_r_type_instr(curr_token.get_instr_data());
                
                } else if (token_kind_is_between(curr_token.kind, I_TYPE_START, I_TYPE_END)){
                    pc += process_i_type_instr(curr_token.get_instr_data());

                } else if (token_kind_is_between(curr_token.kind, S_TYPE_START, S_TYPE_END)){
                    pc += process_s_type_instr(curr_token.get_instr_data());

                } else if (token_kind_is_between(curr_token.kind, B_TYPE_START, B_TYPE_END)){
                    pc += process_b_type_instr(curr_token.get_instr_data());

                } else if (token_kind_is_between(curr_token.kind, U_TYPE_START, U_TYPE_END)){
                    pc += process_u_type_instr(curr_token.get_instr_data());

                } else if(curr_token.kind == JAL){
                    pc += process_j_type_instr(curr_token.get_instr_data());
                    
                } else if (token_is_pseudo_instr(curr_token.kind)){

                    int num_added_instrs = process_p_instr();

                    if(num_added_instrs > 1){
                        // go through all labels below this pseudo instruction and change the address they point to 
                        for(auto& [label, label_addr] : symbol_table){
                            if(label_addr > pc){label_addr += num_added_instrs - 1;}
                        }
                    }

                    pc += num_added_instrs;

                } else {
                    WARNING("Unhandled token " + std::to_string(curr_token.kind));

                    consume(1);
                }

                process();
            }

            void run(){
                /*
                    do first pass to sort out labels
                */
                reset(); set_labels();

                std::cout << "Symbol table " << std::endl;
                for(const auto& [k, v] : symbol_table){
                    std::cout << k << " " << v << std::endl;
                }
                std::cout << std::endl;

                /*
                    second pass for assembly
                */
                reset(); process();
            }

        private:
            std::ofstream stream;
            
            unsigned int num_tokens = 0;
            unsigned int token_pointer = 0;
            Token curr_token;
            Token next_token;
            Token prev_token;

            std::vector<Token> tokens;

            // symbol table and equ directive table
            std::unordered_map<std::string, U64> symbol_table = {};

            U64 pc = 0ULL;
            U32 current_instr_binary = 0UL;

            std::unordered_map<Token_kind, Token_kind> pseudo_to_base = {
                {BGT, BLT},
                {BLE, BGE},
                {BGTU, BLTU},
                {BLEU, BGEU},
                {BEQZ, BEQ},
                {BNEZ, BNE},
                {BGEZ, BGE},
                {BLEZ, BGE},
                {BGTZ, BLT},
            };
    };

}