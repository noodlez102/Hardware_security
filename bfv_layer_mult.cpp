
#define PROFILE

#include <chrono>
#include <fstream>
#include <iostream>
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

#define HeightA 3
#define WidthA 3
#define HeightB 3
#define WidthB 1

#define WeightPath "../question_3/weights.txt"
#define InputsPath "../question_3/inputs.txt"
#define BiasPath "../question_3/bias.txt"

vector<vector<int64_t>> loadFromFile(const string& path) {
    vector<vector<int64_t>> matrix;
    ifstream file(path);

    string line;
    while (getline(file, line)) {
        vector<int64_t> row;
        istringstream ss(line);
        int64_t val;
        while (ss >> val)
            row.push_back(val);
        if (!row.empty())
            matrix.push_back(row);
    }

    return matrix;
}

vector<vector<int64_t>> generateMatrix(int height, int width) {
    vector<vector<int64_t>> mat(height, vector<int64_t>(width));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[i][j] = rand() % 100 + 1;
        }
    }

    return mat;
}

vector<vector<int64_t>> getDiagonals(const vector<vector<int64_t>>& matrix) {
    int height = matrix.size();
    int width = matrix[0].size();

    vector<vector<int64_t>> mat(height, vector<int64_t>(width));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[i][j] = matrix[j % height][(j + i) % width];
        }
    }

    return mat;
}

vector<vector<int64_t>> rotateMatrix(const vector<vector<int64_t>>& matrix) {
    int height = matrix.size();
    int width = matrix[0].size();

    vector<vector<int64_t>> mat(width, vector<int64_t>(height));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            mat[j][i] = matrix[i][j];
        }
    }

    return mat;
}

void printMatrix(const vector<vector<int64_t>>& mat) {
    for (const auto& row : mat) {
        for (const auto& val : row) {
            cout << val << " ";
        }
        cout << endl;
    }
}

vector<vector<int64_t>> multiplyMatrix(const vector<vector<int64_t>>& A,const vector<vector<int64_t>>& B) {
    int H = A.size();         
    int W = A[0].size();      
    int K = B[0].size();      

    vector<vector<int64_t>> C(H, vector<int64_t>(K, 0));

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
	// if (argc != 4) {
    //     std::cerr << "Usage: " << argv[0] << " <matrix> <vector> " << std::endl;
    //     return 1;
    // }
    TimeVar t;
    double processingTime(0.000);
    
    // Sample Program: Step 1: Set CryptoContext
    CCParams<CryptoContextBFVRNS> parameters;
    parameters.SetPlaintextModulus(536903681);
    parameters.SetMultiplicativeDepth(3);
    parameters.SetMaxRelinSkDeg(3);
    CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(LEVELEDSHE);
    KeyPair<DCRTPoly> keyPair;
    keyPair = cryptoContext->KeyGen();
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);
    //need this to use evalrot
    vector<int32_t> rotations;

    int num_shifts = int(log2(512) - log2(10));
    int shift = 512 / 2;
    for (int i = 0; i < num_shifts; ++i) {
        rotations.push_back(shift);
        // rotations.push_back(shift - num_shifts);
        shift /= 2;
    }

    for (int j = 10; j <= 512; j*=2) {
        rotations.push_back(j);
    }
    for (int j = --512; j <= 512; j++) {
        rotations.push_back(j);
    }

    cryptoContext->EvalRotateKeyGen(keyPair.secretKey, rotations);

    cout << endl;
    cout << "+----------------------------------------------------------------------+" << endl;
    cout << "| OPENFHE: BFV Scheme: Multiplication of Matrix and Vector     |" << endl;
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

    //generate random inputs and print them out
    // vector<int64_t> mat_vals = parsemat(argv[1]);
    // vector<int64_t> vec_vals = parsemat(argv[2]);
    vector<vector<int64_t>> weight = loadFromFile(WeightPath);
    vector<vector<int64_t>> test = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    cout<<"matrix of test: "<<endl;
    printMatrix(test);
    cout<<endl;
    vector<vector<int64_t>> test_diagonal = getDiagonals(test);
    cout<<"matrix of test_diagonal: "<<endl;
    printMatrix(test_diagonal);
    cout<<endl;
    vector<vector<int64_t>> hybridweight = getDiagonals(weight);

    vector<vector<int64_t>> input = loadFromFile(InputsPath);

    vector<vector<int64_t>> bias = loadFromFile(BiasPath);
    vector<vector<int64_t>> rotatedbias  = rotateMatrix(bias);

    vector<vector<int64_t>> Real_Answer = multiplyMatrix(weight, rotateMatrix(input));
    cout<< "got through geting real answer"<<endl;


    vector<Plaintext> plaintextweight;
    vector<Plaintext> plaintextinput;
    vector<Plaintext> plaintextbias;
    //cout<<"matrix of weight: "<<endl;
    // printMatrix(weight);
    // cout<<endl;
    // cout<<"matrix of weight: "<<endl;
    // printMatrix(Real_Answer);
    // cout<<endl;
    // First plaintext vector is encoded
    for(int i = 0; i < hybridweight.size(); i++){
        auto pt = cryptoContext->MakePackedPlaintext(hybridweight[i]);
        plaintextweight.push_back(pt);
        // cout << "plain text of diagonal weight: "<< pt <<endl;

    }
    cout <<endl;

    for(int i = 0; i < input.size(); i++){
        auto pt = cryptoContext->MakePackedPlaintext(input[i]);
        plaintextinput.push_back(pt);
        // cout << "plain text of input: "<< pt <<endl;
    }
    cout <<endl;

    for(int i = 0; i < rotatedbias.size(); i++){
        auto pt = cryptoContext->MakePackedPlaintext(rotatedbias[i]);
        plaintextbias.push_back(pt);
        // cout << "plain text of bias: "<< pt <<endl;
    }
    cout <<endl;

    vector<int64_t> mask(512, 0);
    for (int i = 0; i < 10; ++i) {
        mask[i] = 1;
    }
    Plaintext plainmask = cryptoContext->MakePackedPlaintext(mask);

    // The encryption process
    std::cout << "Encrypting #input ........ "<< std::endl;
    vector<Ciphertext<DCRTPoly>> cipherinput;
    for(int i = 0; i < plaintextinput.size(); i++){
        auto ciphertext1 = cryptoContext->Encrypt(keyPair.publicKey, plaintextinput[i]);
        cipherinput.push_back(ciphertext1);
    }
    
    // Homomorphic multiplications
    TIC(t);

    //first half 

    Ciphertext<DCRTPoly> cipherMult = cryptoContext->EvalMult(cipherinput[0], plaintextweight[0]);
    cout <<"starting first rot and accum"<<endl;
    for(int j = 1; j < 10; j++){ 
        auto ciphertextMulleft      = cryptoContext->EvalRotate(cipherinput[0], -j);
        auto ciphertextMulrot      = cryptoContext->EvalRotate(cipherinput[0], -j + 512);
        auto ciphertextrotFinal = cryptoContext->EvalAdd(ciphertextMulleft, ciphertextMulrot);

        auto ciphertextMultResult = cryptoContext->EvalMult(ciphertextrotFinal, plaintextweight[j]);
        cipherMult=cryptoContext->EvalAdd(cipherMult, ciphertextMultResult);
    }
    //next half needs to rotate and accumulate into a 10x1

    cout <<"starting 2nd rot and accum"<<endl;

    num_shifts = int(log2(512) - log2(10));
    shift = 512 / 2;
    for (int i = 0; i < num_shifts; ++i) {
        auto ciphertextMulleft      = cryptoContext->EvalRotate(cipherMult, shift);
        cipherMult = cryptoContext->EvalAdd(cipherMult, ciphertextMulleft);
        shift /= 2;
    }

    cipherMult = cryptoContext->EvalMult(cipherMult, plainmask);

    // for(int i = 10; i <512; i*=2){
    //     auto cipherrotchunk = cryptoContext->EvalRotate(cipherMult, i);
    //     cipherMult= cryptoContext->EvalAdd(cipherMult,cipherrotchunk);
    // }
    // int chunkSize = 10;
    // int chunks    = 512 / chunkSize; // 51
    // for(int step = 1; step < chunks; step *= 2){
    //     Ciphertext<DCRTPoly> cipherrotchunk = cryptoContext->EvalRotate(cipherMult, step * chunkSize);
    //     cipherMult = cryptoContext->EvalAdd(cipherMult, cipherrotchunk);
    // }
    processingTime = TOC(t);
    std::cout << "Multiplicaton time matrix * Vector: " << processingTime << "ms" << std::endl;
    
    // Decrypt the result of multiplications
    vector<Plaintext> plaintextMultResult;

    for (int i = 0; i < WidthB; i++) {
        Plaintext pt;
        cryptoContext->Decrypt(keyPair.secretKey, cipherMult, &pt);

        plaintextMultResult.push_back(pt);
        pt->SetLength(20);
        std::cout << "Row: " << pt << std::endl;
    }

    cout << "\nReal Answer B:\n";
    printMatrix(Real_Answer);
    return 0;
}
