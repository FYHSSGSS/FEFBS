#define PROFILE

#include "openfhe.h"
#include <functional>
#include "../fbs_utils.h"
using namespace lbcrypto;

int main(int argc, char* argv[]) {
    signal(SIGSEGV, sigsegv_handler);
    std::string sigmoid                    = "1/(1+exp(-x))";
    std::function<double(double)> sigmoidfunc = [](double x) -> double {
        return 1 / (1 + std::exp(-x));
    };

    std::string filename = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--file") {
            if (i + 1 >= argc || (argv[i + 1] && argv[i + 1][0] == '-')) {
                filename = "../coeffs/sigmoidcoeff.txt";
            } else {
                filename = argv[i + 1];
                ++i; 
            }
        }
    }
    FuncBootstrapExample(sigmoid, -8, 8, 34, 8, sigmoidfunc, filename, 32768, 31.34);
    return 0;
}