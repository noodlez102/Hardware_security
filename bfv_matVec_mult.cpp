
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
            mat[i][j] = rand() % 101;
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
    for (int j = 0; j < HeightB; j++) {
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
    vector<vector<int64_t>> matrix = generateMatrix(WidthA, HeightA);
    vector<vector<int64_t>> vecMatrix = generateMatrix(WidthB, HeightB);
    cout << "Matrix A:\n";
    printMatrix(matrix);
    cout << "\nMatrix B:\n";
    printMatrix(vecMatrix);
    vector<vector<int64_t>> reorderedMatrix = getDiagonals(matrix);
    cout<< "got through geting diagonals"<<endl;
    vector<vector<int64_t>> Real_Answer = multiplyMatrix(matrix,vecMatrix);
    cout<< "got through geting real answer"<<endl;

    vector<Plaintext> plaintextsMatrix;
    vector<Plaintext> plaintextVector;

    
    // First plaintext vector is encoded
    for(int i = 0; i < WidthB; i++){
        plaintextVector.push_back(cryptoContext->MakePackedPlaintext(vecMatrix[i]));
    }

    for(int i = 0; i < reorderedMatrix.size(); i++){
        plaintextsMatrix.push_back(cryptoContext->MakePackedPlaintext(reorderedMatrix[i]));
    }

    // The encryption process
    std::cout << "Encrypting #vector ........ "<< std::endl;
    vector<Ciphertext<DCRTPoly>> ciphervector;
    for(int i = 0; i < WidthB; i++){
        auto ciphertext1 = cryptoContext->Encrypt(keyPair.publicKey, plaintextVector[i]);
        ciphervector.push_back(ciphertext1);
    }
    std::cout << "Encrypting #matrix ........ " << std::endl;
    vector<Ciphertext<DCRTPoly>> cipherMatrix; //idk if that type is correct so double check if wrong

    for(int i = 0; i < plaintextsMatrix.size(); i++){
        auto ciphertext2 = cryptoContext->Encrypt(keyPair.publicKey, plaintextsMatrix[i]);
        cipherMatrix.push_back(ciphertext2);
    }
    std::cout << std::endl;
    
    // Homomorphic multiplications
    vector<Ciphertext<DCRTPoly>> cipherMult;
    TIC(t);
    for(int i = 0; i < WidthB; i++){
        Ciphertext<DCRTPoly> total;
        bool first = true;
        for(int j = 0; j< HeightB; j++){
            auto ciphertextMulrot      = cryptoContext->EvalRotate(ciphervector[i], j);
            auto ciphertextMultResult = cryptoContext->EvalMult(cipherMatrix[j], ciphertextMulrot);

            if (first) {
                total = ciphertextMultResult;
                first = false;
            } else {
                total = cryptoContext->EvalAdd(total, ciphertextMultResult);
            }
        }
        cipherMult.push_back(total);
    }

    processingTime = TOC(t);
    std::cout << "Multiplicaton time matrix * Vector: " << processingTime << "ms" << std::endl;
    
    // Decrypt the result of multiplications
    vector<Plaintext> plaintextMultResult;

    for (int i = 0; i < WidthB; i++) {
        Plaintext pt;
        cryptoContext->Decrypt(keyPair.secretKey, cipherMult[i], &pt);

        plaintextMultResult.push_back(pt);

        std::cout << "Row: " << pt << std::endl;
    }

    cout << "\nReal Answer B:\n";
    printMatrix(Real_Answer);
    return 0;
}
