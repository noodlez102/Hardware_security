from math import log2

def matrix_vector_multiply_squat(packed_matrix, vector):
    """Multiply the squat-diagonal-packed matrix by the vector."""
    n, m = len(packed_matrix), len(packed_matrix[0])
    def rotate(lst, shift):
        shift = shift % m
        return lst[shift:] + lst[:shift]
    
    def add(a, b):
        return [x + y for x, y in zip(a, b)]
    
    def mul(a, b):
        return [x * y for x, y in zip(a, b)]

    # Same as Halevi-Shoup
    row_products = []
    for i in range(n):
        row_products.append(packed_matrix[i] * vector.rotate(-i))

    # Same as Halevi-Shoup
    partial_sums = row_products[0]
    for i in range(1, n):
        partial_sums += row_products[i]

    # The rest is specific to the squat packing.
    # Reduce the result to combine partial sums.
    result = partial_sums
    num_shifts = int(log2(m) - log2(n))
    shift = m // 2
    for _ in range(num_shifts):
        result += result.rotate(shift)
        shift //= 2

    # Mask out the first n entries
    mask = [0] * m
    for i in range(n):
        mask[i] = 1

    return result * mask

def pack_squat(matrix):
    """Pack the matrix into a list of ciphertexts via
    Juvekar-Vaikuntanathan-Chandrakasan squat diagonal packing.
    """
    n, m = len(matrix), len(matrix[0])

    ciphertexts = [[None] * m for _ in range(n)]
    for i in range(n):
        for j in range(m):
            ciphertexts[i][j] = matrix[j % n][(i + j) % m]

    return [(ciphertexts[i]) for i in range(n)]

def load_vector_from_file(path):
    with open(path, 'r') as f:
        return list(map(float, f.read().split()))

def load_matrix_from_file(path):
    matrix = []
    with open(path, 'r') as f:
        for line in f:
            row = list(map(float, line.strip().split()))
            if row:
                matrix.append(row)
    return matrix

if __name__ == "__main__":
    matrix = load_matrix_from_file("../question_3/weights.txt")
    vector = load_vector_from_file("../question_3/inputs.txt")

    n, m = len(matrix), len(matrix[0])
    print(f"Matrix: {n}x{m}, Vector: {len(vector)}")

    packed  = pack_squat(matrix)
    result  = matrix_vector_multiply_squat(packed, vector)

    print("Got:     ", [round(x, 4) for x in result[:n]])
