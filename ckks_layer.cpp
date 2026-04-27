#define PROFILE

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <vector>
#include <string>
#include <random>
#include <bits/stdc++.h>

#include "openfhe.h"

using namespace lbcrypto;
using namespace std;


#define WeightPath "../question_3/weights.txt"
#define InputsPath "../question_3/inputs.txt"
#define BiasPath "../question_3/bias.txt"

vector<vector<double>> loadFromFile(const string& path) {
    vector<vector<double>> matrix;
    ifstream file(path);

    string line;
    while (getline(file, line)) {
        vector<double> row;
        istringstream ss(line);
        double val;
        while (ss >> val)
            row.push_back(val);
        if (!row.empty())
            matrix.push_back(row);
    }

    return matrix;
}

vector<vector<double>> generateMatrix(int height, int width) {
    vector<vector<double>> mat(height, vector<double>(width));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[i][j] = rand() % 100 + 1;
        }
    }

    return mat;
}

vector<vector<double>> getDiagonals(const vector<vector<double>>& matrix) {
    int height = matrix.size();
    int width = matrix[0].size();

    vector<vector<double>> mat(height, vector<double>(width));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[i][j] = matrix[j % height][(j + i) % width];
        }
    }

    return mat;
}

vector<vector<double>> rotateMatrix(const vector<vector<double>>& matrix) {
    int height = matrix.size();
    int width = matrix[0].size();

    vector<vector<double>> mat(width, vector<double>(height));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[j][i] = matrix[i][j];
        }
    }

    return mat;
}

void printMatrix(const vector<vector<double>>& mat) {
    for (const auto& row : mat) {
        for (const auto& val : row) {
            cout << val << " ";
        }
        cout << endl;
    }
}

vector<vector<double>> multiplyMatrix(const vector<vector<double>>& A,const vector<vector<double>>& B) {
    int H = A.size();         
    int W = A[0].size();      
    int K = B[0].size();      

    vector<vector<double>> C(H, vector<double>(K, 0));

    for (int i = 0; i < H; i++) {
        for (int j = 0; j < K; j++) {
            for (int k = 0; k < W; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return C;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <num1> <num2> <num3>" << std::endl;
        return 1;
    }

    uint32_t multDepth = 3;
    TimeVar t;
    double processingTime(0.0);
    uint32_t scaleModSize = 50;
    uint32_t batchSize = 1; //set this value to 2^x close to size of vectors you are dealing with
    Plaintext result;
    std::cout.precision(8);

    // Set crypto parameters for CKKS
    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetBatchSize(batchSize);
    CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);
    auto keyPair = cryptoContext->KeyGen();
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);

    vector<int32_t> rotations;

    int num_shifts = int(log2(512) - log2(16));
    int shift = 512 / 2;
    for (int i = 0; i < num_shifts; i++) {
        rotations.push_back(shift);
        // rotations.push_back(shift - num_shifts);
        shift /= 2;
    }
    for (int j = 0; j <= 16; j++) {
        rotations.push_back(j);
        rotations.push_back(j-512);
    }

    cryptoContext->EvalRotateKeyGen(keyPair.secretKey, rotations);

    cout << endl;
    cout << "+----------------------------------------------------------------------+" << endl;
    cout << "| OPENFHE: CKKS Scheme: Multiplication of three inputs     |" << endl;
    cout << "+----------------------------------------------------------------------|" << endl;
    cout << "/" << endl;
    
    cout << endl;
    cout << "Encryption Parameters: " << endl;
    std::cout << "p = " << cryptoContext->GetCryptoParameters()->GetPlaintextModulus() << std::endl;
    std::cout << "n = " << cryptoContext->GetCryptoParameters()->GetElementParams()->GetCyclotomicOrder() / 2
              << std::endl;
    std::cout << "log2 q = "
              << log2(cryptoContext->GetCryptoParameters()->GetElementParams()->GetModulus().ConvertToDouble())
              << std::endl;

    // Parse inputs as doubles (CKKS works on floating-point)
    vector<vector<double>> weight = loadFromFile(WeightPath);
    for (int i = 0; i < 6; ++i) {
        weight.push_back(vector<double>(512, 0));
    }

    vector<vector<double>> hybridweight = getDiagonals(weight);

    vector<vector<double>> input = loadFromFile(InputsPath);

    vector<vector<double>> bias = loadFromFile(BiasPath);
    vector<vector<double>> rotatedbias  = rotateMatrix(bias);

    vector<Plaintext> plaintextweight;
    vector<Plaintext> plaintextinput;
    vector<Plaintext> plaintextbias;

    // Encode inputs
    for(int i = 0; i < hybridweight.size(); i++){
        auto pt = cryptoContext->MakeCKKSPackedPlaintext(hybridweight[i]);
        plaintextweight.push_back(pt);
        // cout << "plain text of diagonal weight: "<< pt <<endl;

    }
    cout <<endl;

    for(int i = 0; i < input.size(); i++){
        auto pt = cryptoContext->MakeCKKSPackedPlaintext(input[i]);
        plaintextinput.push_back(pt);
        // cout << "plain text of input: "<< pt <<endl;
    }
    cout <<endl;

    for(int i = 0; i < rotatedbias.size(); i++){
        auto pt = cryptoContext->MakeCKKSPackedPlaintext(rotatedbias[i]);
        plaintextbias.push_back(pt);
        // cout << "plain text of bias: "<< pt <<endl;
    }
    cout <<endl;

    // Encrypt the encoded vectors
    std::cout << "Encrypting #input ........ "<< std::endl;
    vector<Ciphertext<DCRTPoly>> cipherinput;
    for(int i = 0; i < plaintextinput.size(); i++){
        auto ciphertext1 = cryptoContext->Encrypt(keyPair.publicKey, plaintextinput[i]);
        cipherinput.push_back(ciphertext1);
    }

    // Homomorphic addition
    TIC(t);

    Ciphertext<DCRTPoly> cipherMult;
    // cout <<"starting first rot and accum"<<endl;
    for(int j = 0; j < 16; j++){ 
        auto ciphertextMulleft      = cryptoContext->EvalRotate(cipherinput[0], j);
        auto ciphertextMulrot      = cryptoContext->EvalRotate(cipherinput[0], j - 512);
        auto ciphertextrotFinal = cryptoContext->EvalAdd(ciphertextMulleft, ciphertextMulrot);

        auto ciphertextMultResult = cryptoContext->EvalMult(ciphertextrotFinal, plaintextweight[j]);
        if(j==0){
            cipherMult = ciphertextMultResult;
        }else{
            cipherMult=cryptoContext->EvalAdd(cipherMult, ciphertextMultResult);
        }
    }
    //next half needs to rotate and accumulate into a 10x1

    // cout <<"starting 2nd rot and accum"<<endl;

    num_shifts = int(log2(512) - log2(16));
    shift = 512 / 2;
    for (int i = 0; i < num_shifts; i++) {
        auto ciphertextMulleft      = cryptoContext->EvalRotate(cipherMult, shift);
        cipherMult = cryptoContext->EvalAdd(cipherMult, ciphertextMulleft);
        shift /= 2;
    }
    cipherMult = cryptoContext->EvalAdd(cipherMult, plaintextbias[0]);

    processingTime = TOC(t);
    std::cout << "layer time calculation: " << processingTime << "ms" << std::endl;
    
    // Decrypt the result of addition
    vector<Plaintext> plaintextMultResult;

    for (int i = 0; i < 1; i++) {
        Plaintext pt;
        cryptoContext->Decrypt(keyPair.secretKey, cipherMult, &pt);

        plaintextMultResult.push_back(pt);
        pt->SetLength(10);
        std::cout << "layer: " << pt << std::endl;
    }

    return 0;
}
