#include <unistd.h>
#include "helper_funcs.h"
#include "assembler.h"

int main(int argc, char* argv[]) {
    std::string path = "";

    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                path = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p <path>" << std::endl;
                return 1;
        }
    }

    if (path == "") {
        std::cerr << "Usage: " << argv[0] << " -p <path>" << std::endl;
        std::cerr << "Missing required arguments." << std::endl;
        return 1;
    }

    std::cout << "Path: " << path << std::endl;

    assembler asmbl;

    std::vector<std::string> instrs;

    for (auto p : GetFilesToParse(path)) {
        asmbl.SetCurrentPath(fs::path(p));
        asmbl.assemble();
    }

    return 0;
}