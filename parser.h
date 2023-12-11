#ifndef PARSER_H
#define PARSER_H

#include "helper_funcs.h"

/*
  Given an input path, the parser should return a vector of strings where each string is a RISCV instruction, or a label
  - Take out all comments
*/

class Parser{
    public:
        Parser () {}
        void SetPathToFile(const fs::path& path){
            current_path = path;
        }

        std::vector<std::string> GetInstrs(){
            return instructions;
        }
        
        void Parse(){
            instructions = {};

            std::ifstream infile;
            infile.open(current_path);

            if(!infile.is_open()){
                std::cout << "Error opening file at path : " << current_path << std::endl; 
            }

            std::string line, part;

            while(std::getline(infile, line)){              
                if(!removeWhiteSpace(line).empty() && line[0] != '#'){   // ignore newlines and starting comments
                    //std::cout << line << std::endl;  
                    if(line.find("#") != std::string::npos){
                        part = splitString(removeWhiteSpace(line), "#")[0];
                        if(!part.empty()){instructions.push_back(removeWhiteSpace(part));}
                    } else {
                        instructions.push_back(removeWhiteSpace(line));
                    }

                }
            }

            /*
            for(auto i : instructions){
                std::cout << i << std::endl;
            }*/

        }

    private:
        fs::path current_path;
        std::vector<std::string> instructions;
};

#endif
