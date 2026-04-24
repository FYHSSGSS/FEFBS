# High-Precision Functional Bootstrapping for CKKS from Fourier Extension
This repository contains the implementation of the paper ["High-Precision Functional Bootstrapping for CKKS from Fourier Extension"](https://eprint.iacr.org/2026/367), built on the [OpenFHE](https://github.com/openfheorg/openfhe-development) library.

## Installation
### System Requirements
- Operating System: Linux (Tested and verified on Ubuntu 22.04 and Debian 12).
- Compiler: g++ (C++17 or later).
- Build System: CMake v3.x.

For a comprehensive guide on setting up the environment and installing required system packages on Linux, please refer to [OpenFHE Installation Documentation](https://openfhe-development.readthedocs.io/en/latest/sphinx_rsts/intro/installation/linux.html).

### Python Dependencies
The Fourier coefficient generation relies on Python scripts. Please ensure you have Python 3 installed with the following libraries:
```bash
pip install numpy sympy
```

### Hardware Resource Requirements
To ensure a successful build and execution, please ensure your system meets the following specifications.
#### 1. Compilation
Compiling the project takes approximately 30 minutes on a **single core** (`make -j1`). The peak memory usage during compilation is about 1.5 GB per thread. For parallel builds (e.g., `make -j4`), we recommend at least 2 GB of RAM per core to ensure stability during the linking phase. 
#### 2. Execution
The resource usage depends on the ring dimension $N=2^{16}$ used in our benchmarks.
During execution, the peak memory usage of the artifact is approximately **31 GB** (Maximum Resident Set Size). To ensure stable performance and provide a sufficient safety margin for the operating system and multi-threaded operations, we recommend using a machine with at least **64 GB of RAM**.

### Building the Project
We use the standard CMake build workflow.
```bash
mkdir build
cd build
cmake ..
make -j$(nproc) # Or make -j4 for lower-memory machines
```
Our test binaries will be generated in the `build/examples/pke` directory.

## Homomorphic Encryption Benchmarking
Our test experiments are designed to perform functional bootstrapping on CKKS ciphertexts. The primary goal is to simultaneously raise the ciphertext level and evaluate functions with high precision in a single, efficient procedure.

The source code for our test experiments is located in `src/pke/examples/FEFBS/`, which includes test files for various benchmarking examples. (e.g. $\mathrm{gelu}(x)$, $\exp(x)$) 

After a successful build, the corresponding executable binaries are generated in the `build/bin/examples/pke/` directory.

The test binaries support two modes of operation for obtaining the necessary Fourier coefficients:
### 1. Online Coefficient Generation (Python Dependency)
Our framework invokes `fourier_calculator.py` at runtime to calculate Fourier coefficients. Therefore, you must specify the PYTHONPATH environment variable so the binary can locate the script.

To run a test with online generation (e.g., the exp function):
```bash
# Replace [script_path] with the directory of fourier_calculator.py
PYTHONPATH=[script_path] ./build/bin/examples/pke/exp_test
```
For example, If you are currently inside the `build` directory, you can simply run the following command:
```bash
PYTHONPATH=../ ./build/bin/examples/pke/exp_test
```
The expected output should look like this:
```bash
CKKS scheme is using ring dimension 65536

[Pre-computation] Generating Fourier series coefficients for the target function...
[Pre-computation] Done. 

CKKS total modulus: 1535 bits
Level before bootstrapping: 3

Level after bootstrapping: 8
Total time: 15836 ms
Slots amortize time: 0.483276 ms

--- Sample Points Inspection (Total 10 points) ---
-0.5000000000 -0.4000244141 -0.3000183105 -0.2000122070 -0.1000061035 0.0000000000 0.0999755859 0.1999816895 0.2999877930 0.3999938965 
----------- Expected Function Values: -----------
0.1353352832 0.2018768025 0.3011721526 0.4493070248 0.6703036810 1.0000000000 1.4916790187 2.2253779310 3.3199548116 4.9529115022 
------- Functional Bootstrapping Results: -------
0.1353352832 0.2018768024 0.3011721525 0.4493070248 0.6703036810 0.9999999998 1.4916790190 2.2253779328 3.3199548119 4.9529115014
--------------------------------------------------

Precision: 33.6765 bits
```

### 2. Using Pre-computed Coefficients
For complex functions like GeLU, symbolic pre-computation via `SymPy` can be time-consuming. To save time, we provide pre-generated coefficient files in the `coeffs/` directory.

You can bypass the coefficients generation by providing the coefficient file path using the -f flag: It supports two modes:
- Simply add `-f`. The binary will automatically load its respective pre-computed coefficient file from the `../coeffs/` directory (e.g., gelu_test will look for `gelucoeff.txt`, and exp_test for `expcoeff.txt`).
- Provide a specific path to load a custom coefficient file.
```bash
# Run GeLU with pre-computed coefficients in default pre-computed file
./bin/examples/pke/gelu_example -f

# Example: Running GeLU with pre-computed coefficients in specified file path
./build/bin/examples/pke/gelu_test -f ../coeffs/gelucoeff.txt
```

## Plaintext Prototype
In addition to the HE implementation, we provide a Python-based plaintext simulation tool in the project root directory: `fourier_extension.py`, which is used to prototype the Fourier Extension algorithm and find efficient parameters before running the homomorphic version.

### Usage & Configuration
The Parameters of the function can be provided directly via command-line arguments:

- `--func`: Target function string (e.g., `"x"`, `"exp(x)"`).
- `--left` / `--right`: Evaluation interval $[a, b]$.
- `--degree`: Truncation degree $n$ of the Fourier series.

### Examples
1. Run with default parameters ($f(x)=x,x\in[-1,1],n=40$):
```bash
python fourier_extension.py
```
The script will output the recommended smoothness parameter, the estimated bit precision, and the generated coefficients:
```bash
ke: 33, Precision: 50.5166 bits
Fourier Coefficients (a_n, b_n): [(-3.885780586188048e-18+0j), (-1.8041124150158794e-18-0.625001603545843j), ....]
```
2. Evaluate a custom function with specific parameters, like $f(x)=e^x, x\in[-2,2],n=20$:
```bash
python fourier_extension.py --func "exp(x)" --left -2 --right 2 --degree 20
```

# Evaluation workflow

Here, we describe how to reproduce the experimental results and map the console outputs to the tables presented in the paper.

### 1. Reproducing Table 1 (Standard Bootstrapping)
Table 1 evaluates the efficiency and precision of our method for the standard bootstrapping operator ($f(x)=x$).
- Execution: Run the `bin/examples/pke/boot_test` binary with the `-s` (or `--slots`) flag to test different slot configurations:
    ```bash
    # To reproduce rows with 16,384 or 8,192 slots:
    ./bin/examples/pke/boot_test -s 16384 -f
    ./bin/examples/pke/boot_test -s 8192 -f

    # To reproduce the row with 32,768 slots:
    ./bin/examples/pke/boot_test -s 32768 -f
    ```
- Data Mapping: 
    * The Slots column in Table 1: Specified by the `-s` or `--slots` flag.
    * The Total Latency in Table 1：Corresponds to `Total time: xxx ms` in the output.
    * The Amortized Latency in Table 1: Corresponds to `Slots amortize time: xxx ms` in the output.
    * The precision in Table 1: Corresponds to `Precision: xxx bits` in the output.

### 2. Reproducing Table 3 (Functional Bootstrapping)
Table 3 showcases our framework's ability to evaluate various functions with high precision during the bootstrapping process.
- Execution: Run the corresponding binary for the target function. For example, to test the Exp function:
    ```bash
    ./bin/examples/pke/exp_test -f
    ```
- Data Mapping: Refer to the FuncBootstrapExample call in the source code (`src/pke/examples/FEFBS/fbs_utils.h`):
    ```cpp
    void FuncBootstrapExample(std::string func, double lower_bound, double upper_bound, int N, int speed, std::function<double(double)> target, std::string filename, size_t slots_num)
    ```
    * Truncation Degree $n$ mapped to `int N` in the code.
    * Smoothness Parameter ($\kappa_E$) mapped to `int speed` in the code.
    
    For example, in `src/pke/examples/FEFBS/exp_test.cpp`, you will find the function call:
    ```cpp
    // Paper row: exp, n=29, ke=18
    FuncBootstrapExample("exp", -2.0, 2.0, 29, 18, target_func, ...);
    ```
    Similar to Table 1, the Total Latency, Amortized Latency and Precision columns map directly to `Total time: xxx ms`, `Slots amortize time: xxx ms` and `Precision: xxx bits` in the outputs, respectively. 

### Reproduction of Comparison Results
- [BMTPH21]: This work is natively supported by the OpenFHE library. We have provided the benchmark source code for this baseline at `src/pke/examples/FEFBS/bootstrapping_test/BMTPH21_test.cpp`, which can be verified by running the binary at `/bin/examples/pke/BMTPH21_test`.
- [BKSS25] and [AKP25] (Table 3): This is an LUT-based method where **precision** is analytically as the logarithm of the degree polynomial $\tau$. Since [BKSS25] shares the same underlying bootstrapping pipeline as our work, differing only in the degree of the polynomial, we measured its **latency** by configuring our implementation with their specific degree parameters. This equivalent complexity setup ensures a fair comparison on the same hardware.
- [CHKS25]: As this work introduces a fundamentally different paradigm and remains closed-source, we directly cited the performance and precision metrics from the results reported in Table 3 of [CHKS25].

### Runtime Expectations
The execution time varies based on the function complexity and whether the Fourier coefficients are generated online. The following estimated runtime were measured on a server equipped with an Intel(R) Xeon(R) Gold 5318Y CPU @ 2.10GHz (96 logical cores).
- Using Pre-computed Coefficients (Recommended): ~32 seconds (key generation + Loading coefficients from file + encryption + FBS + decryption).
- Online Generation (Simple Functions): ~50 seconds (key generation + online coefficients generation by python script + encryption + FBS + decryption).
- Online Generation (Complex Functions, e.g., GeLU): ~10 minutes (key generation + online coefficients generation by python script + encryption + FBS + decryption). 

## License
This project is distributed under the BSD 2-Clause License.