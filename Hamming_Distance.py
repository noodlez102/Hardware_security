import pandas as pd
import itertools

FILE = "CRP.xlsx"   
BIT_LENGTH = 32
NUM_PUFS = 16

def hamming(a, b):
    return sum(c1 != c2 for c1, c2 in zip(a, b))

df = pd.read_excel(FILE)

responses = df.iloc[:, 1:]   

num_challenges = len(responses)

print("Challenges:", num_challenges)
print("PUFs:", len(responses.columns))
print()


print("Single-Chip Hamming Distance")
print("--------------------------------")

single_results = {}

for chip in responses.columns:
    chip_responses = responses[chip].tolist()

    distances = []

    for a, b in itertools.combinations(chip_responses, 2):
        hd = hamming(a, b) / BIT_LENGTH
        distances.append(hd)

    avg_hd = sum(distances) / len(distances)
    single_results[chip] = avg_hd

print("PUF\tAvg HD")
for chip, hd in single_results.items():
    print(f"{chip}\t{hd:.4f}")

print()

print("Multi-Chip Hamming Distance")
print("--------------------------------")

chips = responses.columns
distances = []

for challenge_index in range(num_challenges):

    row = responses.iloc[challenge_index]

    for chip1, chip2 in itertools.combinations(chips, 2):
        r1 = row[chip1]
        r2 = row[chip2]

        hd = hamming(r1, r2) / BIT_LENGTH
        distances.append(hd)

multi_chip_hd = sum(distances) / len(distances)

print("Average Multi-Chip HD:", round(multi_chip_hd, 4))
print()
