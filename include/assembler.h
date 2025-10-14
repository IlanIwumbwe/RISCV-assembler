#pragma once

#include <lex.h>

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
                    ERROR("Expected kind: " + std::to_string(kind) + " Current token not matched");
                    std::cout << curr_token << std::endl;
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
                to_file(stream, current_instr_binary);
                current_instr_binary = 0UL;
            }

            void set_register(Register_port port, U32 _reg_num = 32){
                
                U32 reg_num;

                if(_reg_num == 32){
                    assert(token_is_reg(curr_token.kind));
                    reg_num = std::get<U32>(curr_token.value);

                } else {
                    reg_num = reg_num;
                }
                
                switch(port){
                    case RD : current_instr_binary |= (reg_num << 7); break; 
                    case RS1 : current_instr_binary |= (reg_num << 15); break; 
                    case RS2 : current_instr_binary |= (reg_num << 20); break; 
                }

                consume(1);
            }

            U64 get_imm(bool calc_pc_offset = false){

                U64 addr = 0;

                if(curr_token.kind == HEX || curr_token.kind == INT){
                    // address to jump to given, calculate offset directly
                    const char* addr_str = std::get<std::string>(curr_token.value).c_str();
                    char* endptr;

                    addr = std::strtoul(addr_str, &endptr, 0);
                
                } else if (curr_token.kind == LABEL_DECL){
                    addr = symbol_table[std::get<std::string>(curr_token.value) + ":"];
                
                } else {
                    std::cout << "Getting imm from " << curr_token << std::endl;

                    ERROR("Only HEX, INT, LABEL_DECL can give imm!");
                    return 0;
                }

                return (calc_pc_offset) ? (addr - pc) * 4 : addr;
            }

            int process_r_type_instr(const Instruction_data& instr_data){
                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);
                place_bits(current_instr_binary, instr_data.get_funct7(), 25, 7);

                consume(1);

                set_register(RD);
                consume(COMMA);

                set_register(RS1);
                consume(COMMA);

                set_register(RS2);

                return 1;
            }

            int process_i_type_instr(const Instruction_data& instr_data){

                U32 opcode = instr_data.get_opcode();
                U64 imm;
                
                current_instr_binary |= opcode;
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);

                consume(1);

                set_register(RD);

                consume(COMMA);

                if(opcode == 0b0000011){
                    
                    imm = get_imm();

                    consume(1);

                    consume(LBRACK);

                    set_register(RS1);

                    consume(RBRACK);
                    
                } else if (opcode == 0b0010011 || opcode == 0b1100111){

                    set_register(RS1);
                    
                    consume(COMMA);
                    
                    imm = get_imm(); 

                    consume(1);
                }

                place_bits(current_instr_binary, get_bits(imm, 11, 0), 20, 12);

                return 1;
            }

            int process_s_type_instr(const Instruction_data& instr_data){

                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);

                consume(1);

                set_register(RS2);
                consume(COMMA);

                U64 imm = get_imm();

                std::cout << HEX(imm) << std::endl;

                place_bits(current_instr_binary, get_bits(imm, 4, 0), 7, 5);
                place_bits(current_instr_binary, get_bits(imm, 11, 5), 25, 7);

                consume(1);

                consume(LBRACK);
                set_register(RS1);
                consume(RBRACK);

                return 1;
            }

            int process_b_type_instr(const Instruction_data& instr_data){
                
                current_instr_binary |= instr_data.get_opcode();
                place_bits(current_instr_binary, instr_data.get_funct3(), 12, 3);

                consume(1);

                set_register(RS1);
                consume(COMMA);

                set_register(RS2);
                consume(COMMA);

                U64 imm = get_imm(true);

                place_bits(current_instr_binary, get_bits(imm, 11, 11), 7, 1);
                place_bits(current_instr_binary, get_bits(imm, 4, 1), 8, 4);
                place_bits(current_instr_binary, get_bits(imm, 10, 5), 25, 6);
                place_bits(current_instr_binary, get_bits(imm, 12, 12), 31, 1);

                consume(1);

                return 1;
            }

            int process_u_type_instr(const Instruction_data& instr_data){

                current_instr_binary |= instr_data.get_opcode();

                consume(1);

                set_register(RD);
                consume(COMMA);

                U64 imm = get_imm();

                place_bits(current_instr_binary, get_bits(imm, 31, 12), 12, 10);

                consume(1);

                return 1;
            }

            int process_j_type_instr(const Instruction_data& instr_data){

                current_instr_binary |= instr_data.get_opcode();

                consume(1);
                
                set_register(RD);
                consume(COMMA);

                U64 imm = get_imm(true);

                // imm[20|10:1|11|19:12]
                place_bits(current_instr_binary, get_bits(imm, 19, 12), 12, 8);
                place_bits(current_instr_binary, get_bits(imm, 11, 11), 20, 1);
                place_bits(current_instr_binary, get_bits(imm, 10, 1), 21, 10);
                place_bits(current_instr_binary, get_bits(imm, 20, 20), 31, 1);

                consume(1);

                return 1;
            }

            int process_p_instr(){

                if(curr_token.kind == LI){
                    consume(1);

                    set_register(RD);

                    consume(COMMA);

                    U64 imm = get_imm();
                    U64 lower_12_bits = imm & 4095;

                    consume(1);

                    if((imm > 4095) && lower_12_bits){
                        // add lui instr
                        process_u_type_instr(find_instr_data_for(LUI));

                    } 
                    
                    // add addi

                    return 2;
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
                    pc += process_r_type_instr(std::get<Instruction_data>(curr_token.value));
                
                } else if (token_kind_is_between(curr_token.kind, I_TYPE_START, I_TYPE_END)){
                    pc += process_i_type_instr(std::get<Instruction_data>(curr_token.value));

                } else if (token_kind_is_between(curr_token.kind, S_TYPE_START, S_TYPE_END)){
                    pc += process_s_type_instr(std::get<Instruction_data>(curr_token.value));

                } else if (token_kind_is_between(curr_token.kind, B_TYPE_START, B_TYPE_END)){
                    pc += process_b_type_instr(std::get<Instruction_data>(curr_token.value));

                } else if (token_kind_is_between(curr_token.kind, U_TYPE_START, U_TYPE_END)){
                    pc += process_u_type_instr(std::get<Instruction_data>(curr_token.value));

                } else if(curr_token.kind == JAL){
                    pc += process_j_type_instr(std::get<Instruction_data>(curr_token.value));
                    
                } else if (token_is_pseudo_instr(curr_token.kind)){

                    // num_base_instructions_processed += process_p_instr();

                    consume(1); // TODO: remove this later
                
                } else {
                    // WARNING("Unhandled token " + std::to_string(curr_token.kind));
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
            std::unordered_map<std::string, int> imm_bindings;

            U64 pc = 0ULL;
            U32 current_instr_binary = 0UL;
    };

}