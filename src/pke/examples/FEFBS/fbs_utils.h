#include <vector>
#include <iostream>
#include "openfhe.h"
#include "math/fourier.h"
#include <functional>
#include <chrono>
#include <string>
#include <complex>
#include <csignal>
#include <unistd.h> 

void sigsegv_handler(int sig) {
    const char* msg =
        "\n[FATAL ERROR] Segmentation fault occurred.\n"
        "Possible causes:\n"
        "1. Python is not installed.\n"
        "2. Dependencies (sympy, numpy) are missing. Please install them via pip:\n"
        "   pip install numpy sympy\n"
        "3. PYTHONPATH is not set correctly to find fourier_calculator.py. Please ensure your command includes PYTHONPATH and points to the project root.\n";
    ssize_t _ = write(STDERR_FILENO, msg, strlen(msg));
    (void)_;
    _exit(1); 
}

namespace lbcrypto {
    inline std::vector<std::complex<double>> LoadCoeffs(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            OPENFHE_THROW("Cannot open coefficient file: " + filename);
        }
        size_t numCoeffs = 0;
        if (!(file >> numCoeffs)) {
            OPENFHE_THROW("Failed to read the number of coefficients from: " + filename);
        }
        std::vector<std::complex<double>> coeffs(numCoeffs + 1);    
        std::cout << numCoeffs + 1 << " coefficients will be loaded from the file." << std::endl;
        double real, imag;
        for (size_t i = 0; i <= numCoeffs; i++) {
            file >> real >> imag;
            coeffs[i] = std::complex<double>(real, imag);
        }
        return coeffs;
    }

    void FuncBootstrapExample(std::string func, double lower_bound, double upper_bound, int N, int speed,
                          std::function<double(double)> target, std::string filename, size_t slots_num, double claimedPrecision) {
        auto ringDim = 1 << 16;
        CCParams<CryptoContextCKKSRNS> parameters;
        CKKSDataType ckksDataType = COMPLEX;
        parameters.SetCKKSDataType(ckksDataType);
        SecretKeyDist secretKeyDist       = SPARSE_TERNARY;
        ScalingTechnique rescaleTech      = FLEXIBLEAUTO;
        usint dcrtBits                    = 59;
        usint firstMod                    = 60;
        size_t numSlots                   = slots_num;
        std::vector<uint32_t> levelBudget = {3, 2};
        std::vector<uint32_t> bsgsDim     = {0, 0};
        usint depth                       = levelBudget[0] + levelBudget[1] + 12 + 9;
        parameters.SetMultiplicativeDepth(depth);

        parameters.SetSecretKeyDist(secretKeyDist);
        parameters.SetSecurityLevel(HEStd_NotSet);
        parameters.SetRingDim(ringDim);
        parameters.SetNumLargeDigits(3);
        parameters.SetKeySwitchTechnique(HYBRID);
        parameters.SetScalingModSize(dcrtBits);
        parameters.SetScalingTechnique(rescaleTech);
        parameters.SetFirstModSize(firstMod);
        parameters.SetBatchSize(numSlots);
        CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);

        cc->Enable(PKE);
        cc->Enable(KEYSWITCH);
        cc->Enable(LEVELEDSHE);
        cc->Enable(ADVANCEDSHE);
        cc->Enable(FHE);

        printf("CKKS scheme is using ring dimension %d\n", ringDim);
        cc->EvalFEFuncBootstrapSetup(levelBudget, bsgsDim, numSlots);
        auto keyPair = cc->KeyGen();
        cc->EvalMultKeyGen(keyPair.secretKey);
        cc->EvalBootstrapKeyGen(keyPair.secretKey, numSlots);

        constexpr double left = -0.5;
        constexpr double mid  = 0.5;
        std::vector<double> x(numSlots);
        std::vector<std::complex<double>> coeffspython;
        if (filename != "") {
            printf("[Config] Loading coefficients from file: %s\n", filename.c_str());
            coeffspython = LoadCoeffs(filename);
            puts("[Config] Done. ");    
        } else {
            puts("\n[Pre-computation] Generating Fourier series coefficients for the target function...");
            FourierCalculator fourierCalc;
            coeffspython = fourierCalc.calculate(func, lower_bound, upper_bound, N, speed + 1);
            puts("[Pre-computation] Done. ");
        }
        for (size_t i = 0; i < numSlots; i++) {
            x[i] = left + static_cast<double>(i) * (mid - left) / static_cast<double>(numSlots);
        }
        int numSamples = 10;
        

        double middle_point = (lower_bound + upper_bound) / 2.0;
        std::vector<double> y(x.size());
        for (size_t i = 0; i < x.size(); ++i) {
            y[i] = target((upper_bound - lower_bound) * (x[i] - middle_point));
        }

        Plaintext ptxt = cc->MakeCKKSPackedPlaintext(x, 1, depth - (levelBudget[1] + 1), nullptr, numSlots);
        Ciphertext<DCRTPoly> ctxt = cc->Encrypt(keyPair.publicKey, ptxt);
        printf("\nCKKS total modulus: %d bits\n", firstMod + (depth - 1) * dcrtBits);
        printf("Level before bootstrapping: %u\n\n", (uint32_t)(depth - ctxt->GetLevel()));
        std::chrono::system_clock::time_point start, end;
        start           = std::chrono::system_clock::now();
        auto ctxtResult = cc->EvalFEFuncBootstrap(ctxt, coeffspython);
        end             = std::chrono::system_clock::now();
        printf("Level after bootstrapping: %u\n", (uint32_t)(depth - ctxtResult->GetLevel()));
        printf("Total time: %ld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        printf("Slots amortize time: %.6lf ms\n", (std::chrono::duration_cast<std::chrono::milliseconds>(end - start) / (double)x.size()).count());
        
        Plaintext result;
        cc->Decrypt(keyPair.secretKey, ctxtResult, &result);
        result->SetLength(x.size());
        std::vector<double> resultvec = result->GetRealPackedValue();

        printf("\n--- Sample Points Inspection (Total %d points) ---\n", numSamples);
        for (int i = 0; i < numSamples; i++) {
            printf("%.10lf ", x[i * numSlots / numSamples]);
        }
        printf("\n----------- Expected Function Values: -----------\n");
        for (int i = 0; i < numSamples; i++) {
            printf("%.10lf ", y[i * numSlots / numSamples]);
        }
        printf("\n------- Functional Bootstrapping Results: -------\n");
        for (int i = 0; i < numSamples; i++) {
            printf("%.10lf ", resultvec[i * numSlots / numSamples]);
        }
        puts("\n--------------------------------------------------");
        printf("\nPrecision: %.2lf bits\n", result->GetOutputPrecisionWithClaim(y, claimedPrecision)); 
    }

}