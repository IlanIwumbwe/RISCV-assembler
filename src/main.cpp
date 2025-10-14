#include <unistd.h>
#include <assembler.h>
#include <lex.h>

int main(int argc, char* argv[]) {
    std::string path = "";

    for(auto& file : fs::directory_iterator(fs::path("../assembly_files"))){

        if(file.is_regular_file() && (file.path().extension() == ".asm")){          
            std::cout << "Assembling: " << file.path().string() << std::endl;    
            
            Assembler::Lexer lexer(file.path().string());
            // lexer.print_tokens();

            auto outpath = file.path();

            Assembler::Assembler assembler(lexer.get_tokens(), outpath);
            assembler.run();
        }
    }

    return 0;
}