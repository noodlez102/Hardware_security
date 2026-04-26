
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

#include "openfhe.h"

using namespace lbcrypto;
using namespace std;

#define HeightA 6
#define WidthA 6
#define HeightB 6
#define WidthB 1


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
            mat[i][j] = matrix[j][(j + i) % width];
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
    for (int j = -HeightB; j <= HeightB; j++) {
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
    vector<vector<int64_t>> vecMatrix = generateMatrix(HeightB, WidthB);
    vector<vector<int64_t>> rotatedVecMatrix = rotateMatrix(vecMatrix);
    cout << "\nMatrix B:\n";
    printMatrix(vecMatrix);
    cout << "\nMatrix B but rotated:\n";
    printMatrix(rotatedVecMatrix);

    vector<Plaintext> plaintextVector;


    // First plaintext vector is encoded
    for(int i = 0; i < WidthB; i++){
        auto pt = cryptoContext->MakePackedPlaintext(rotatedVecMatrix[i]);
        plaintextVector.push_back(pt);
        cout << "plain text of vector: "<< pt;
    }
    cout <<endl;
    // The encryption process
    std::cout << "Encrypting #vector ........ "<< std::endl;
    vector<Ciphertext<DCRTPoly>> ciphervector;
    for(int i = 0; i < WidthB; i++){
        auto ciphertext1 = cryptoContext->Encrypt(keyPair.publicKey, plaintextVector[i]);
        ciphervector.push_back(ciphertext1);
    }
    // Homomorphic multiplications
    vector<Ciphertext<DCRTPoly>> cipherMult;
    TIC(t);

    auto ciphertextMulleft      = cryptoContext->EvalRotate(ciphervector[0], 1);
    cipherMult.push_back(ciphertextMulleft);

    for(int i =1; i< HeightB; i++){
        cryptoContext->EvalRotate(ciphertextMulleft, i);
        cipherMult.push_back(ciphertextMulleft);
    }
    // auto ciphertextMulrot      = cryptoContext->EvalRotate(ciphervector[0], 1 - HeightB);
    // auto ciphertextrotFinal = cryptoContext->EvalAdd(ciphertextMulleft, ciphertextMulrot);
    // cipherMult.push_back(ciphertextrotFinal);


    processingTime = TOC(t);
    std::cout << "Multiplicaton time matrix * Vector: " << processingTime << "ms" << std::endl;
    
    // Decrypt the result of multiplications
    for(int i =0; i< HeightB;i++){
        Plaintext pt;
        cryptoContext->Decrypt(keyPair.secretKey, cipherMult[i], &pt);
        pt->SetLength(HeightB);
        std::cout << "Row: " << pt << std::endl;
    }


    return 0;
}
