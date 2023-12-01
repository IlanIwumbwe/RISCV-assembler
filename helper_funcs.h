#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

#include <regex>

// common imports
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <bitset>

namespace fs = std::filesystem;

/// Given a path, return a vector of path(s) that contain assembly. Input path could be folder of assembly files, or a single assembly file.
std::vector<std::string> GetFilesToParse(std::string& path){
    std::vector<std::string> paths;

    if (!fs::is_directory(path)){
        paths.push_back(path);
        return paths;
    } else {
        for (const auto& entry : fs::directory_iterator(path))
        {   
            if (entry.path().extension() == ".asm" || entry.path().extension() == ".s"){
                paths.push_back(entry.path().string());
            }
        }
    }

    return paths;
}

std::vector<std::string> splitString(const std::string& input, const std::string& delimiter){
    std::vector<std::string> result;
    std::regex regexDelimiter(delimiter);
    std::sregex_token_iterator iterator(input.begin(), input.end(), regexDelimiter, -1);

    // Use sregex_token_iterator to split the string and add tokens to the result vector
    while (iterator != std::sregex_token_iterator()) {
        result.push_back(*iterator);
        ++iterator;
    }

    return result;
}

inline std::string removeWhiteSpace(std::string str){
    std::regex pattern(R"(^\s+|\s+$)");

    return std::regex_replace(str, pattern, "");
}

inline auto numtobin(int number){
    std::bitset<sizeof(number) * 8> binaryRepresentation(number);

    return binaryRepresentation;
}

std::string to_lower(const std::string input){
    std::string input_lower = input;

    for (auto& x : input_lower){
        x = (char)tolower(x);
    }
    return input_lower;
}

std::vector<std::string> GetTokens(std::string& input, std::regex pattern){
    // instr = removeWhiteSpace(instr);
    std::vector<std::string> tokens = {};

    std::sregex_iterator iter(input.begin(), input.end(), pattern);
    std::sregex_iterator end;

    while (iter != end) {
        if (iter->str().size() != 0) tokens.push_back(to_lower(iter->str()));
        ++iter;
    }

    return tokens;
}

#endif
