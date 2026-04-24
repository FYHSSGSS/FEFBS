#define PROFILE

#include "openfhe.h"
#include <functional>
#include "../fbs_utils.h"
using namespace lbcrypto;

int main(int argc, char* argv[]) {
    signal(SIGSEGV, sigsegv_handler);
    std::string exp = "exp(x)";
    std::function<double(double)> expfunc = [](double x) -> double {
        return std::exp(x);
    };
    
    std::string filename = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--file") {
            if (i + 1 >= argc || (argv[i + 1] && argv[i + 1][0] == '-')) {
                filename = "../coeffs/expcoeff.txt";
            } else {
                filename = argv[i + 1];
                ++i; 
            }
        }
    }
    FuncBootstrapExample(exp, -2, 2, 29, 18, expfunc, filename, 32768, 32.23);
    return 0;
}