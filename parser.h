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
        void SetPathToFile(std::string& path){
            current_path = path;
        }

        std::vector<std::string> GetInstrs(){
            return instructions;
        }
        
        void Parse(){
            std::ifstream infile;
            infile.open(current_path);

            if(!infile.is_open()){
                std::cout << "Error opening file at path : " << current_path << std::endl; 
            }

            std::string line;

            while(std::getline(infile, line)){                
                if(line.size() != 0){   // ignore newlines
                    if(line.find("#") != std::string::npos){
                        line = splitString(line, "#")[0];
                    }

                    instructions.push_back(removeWhiteSpace(line));
                }
            }
        }

    private:
        std::string current_path;
        std::vector<std::string> instructions;
};

#endif
