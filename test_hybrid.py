
from math import log2

def matrix_vector_multiply_squat(packed_matrix, vector):
    n, m = len(packed_matrix), len(packed_matrix[0])
    
    def rotate(lst, shift):
        shift = shift % m
        return lst[shift:] + lst[:shift]
    
    def add(a, b):
        return [x + y for x, y in zip(a, b)]
    
    def mul(a, b):
        return [x * y for x, y in zip(a, b)]

    # Step 1
    partial_sums = mul(packed_matrix[0], vector)
    for i in range(1, n):
        partial_sums = add(partial_sums, mul(packed_matrix[i], rotate(vector, -i)))

    # Step 2
    result = partial_sums
    num_shifts = int(log2(m) - log2(n))
    shift = m // 2
    for _ in range(num_shifts):
        result = add(result, rotate(result, shift))
        shift //= 2

    # Step 3 - mask
    mask = [1 if i < n else 0 for i in range(m)]
    return mul(result, mask)

def pack_squat(matrix):
    n, m = len(matrix), len(matrix[0])
    ciphertexts = [[None] * m for _ in range(n)]
    for i in range(n):
        for j in range(m):
            ciphertexts[i][j] = matrix[j % n][(i + j) % m]
    return [ciphertexts[i] for i in range(n)]

def load_matrix_from_file(path):
    matrix = []
    with open(path, 'r') as f:
        for line in f:
            row = list(map(float, line.strip().split()))
            if row:
                matrix.append(row)
    return matrix

def load_vector_from_file(path):
    with open(path, 'r') as f:
        return list(map(float, f.read().split()))

def naive_multiply(matrix, vector):
    return [sum(matrix[i][j] * vector[j] for j in range(len(vector))) for i in range(len(matrix))]

if __name__ == "__main__":
    matrix = load_matrix_from_file("../question_3/weights.txt")
    vector = load_vector_from_file("../question_3/inputs.txt")

    n, m = len(matrix), len(matrix[0])
    print(f"Matrix: {n}x{m}, Vector: {len(vector)}")

    packed  = pack_squat(matrix)
    result  = matrix_vector_multiply_squat(packed, vector)
    expected = naive_multiply(matrix, vector)

    print("\nExpected:", [round(x, 4) for x in expected])
    print("Got:     ", [round(x, 4) for x in result[:n]])

    errors = [abs(result[i] - expected[i]) for i in range(n)]
    print(f"\nMax error: {max(errors):.6f}")
    print("PASSED" if max(errors) < 1e-6 else "FAILED")