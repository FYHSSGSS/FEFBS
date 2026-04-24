#define PROFILE

#include "openfhe.h"
#include <functional>
#include "../fbs_utils.h"
using namespace lbcrypto;

int main(int argc, char* argv[]) {
    signal(SIGSEGV, sigsegv_handler);
    std::string boot = "x";
    size_t numSlots = 32768;
    std::function<double(double)> bootfunc = [](double x) -> double {
        return x;
    };

    std::string filename = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--file") {
            if (i + 1 >= argc || (argv[i + 1] && argv[i + 1][0] == '-')) {
                filename = "../coeffs/bootcoeff.txt";
            } else {
                filename = argv[i + 1];
                ++i; 
            }
        }
        else if (arg == "-s" || arg == "--slots") {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                numSlots = std::stoul(argv[i + 1]);
                ++i;
            }
        }
    }
    FuncBootstrapExample(boot, -0.5, 0.5, 25, 17, bootfunc, filename, numSlots, 31.20);
    return 0;
}