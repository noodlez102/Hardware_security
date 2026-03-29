import os
import re
import pandas as pd

# Base directory (change if needed)
base_dir = "."

netlists = ["A", "B", "C", "D"]

data = []

for n in netlists:
    folder = f"netlist_{n}"
    stats_path = os.path.join(base_dir, folder, "stats.txt")

    if not os.path.exists(stats_path):
        print(f"Missing: {stats_path}")
        continue

    with open(stats_path, "r") as f:
        text = f.read()

    # Extract total cells
    cells_match = re.search(r"Number of cells:\s+(\d+)", text)
    cells = int(cells_match.group(1)) if cells_match else None

    # Extract timing
    timing_match = re.search(r"Timing estimate:\s+([\d\.]+)\s+ns\s+\(([\d\.]+)\s+MHz\)", text)
    delay = float(timing_match.group(1)) if timing_match else None
    freq = float(timing_match.group(2)) if timing_match else None

    # Extract specific cell counts (optional but useful)
    lut_match = re.search(r"SB_LUT4\s+(\d+)", text)
    dff_match = re.search(r"SB_DFF\s+(\d+)", text)

    lut = int(lut_match.group(1)) if lut_match else 0
    dff = int(dff_match.group(1)) if dff_match else 0

    data.append({
        "Netlist": n,
        "Total Cells": cells,
        "LUT4 Count": lut,
        "DFF Count": dff,
        "Delay (ns)": delay,
        "Freq (MHz)": freq
    })

# Create DataFrame
df = pd.DataFrame(data)

# Save to Excel
output_file = "netlist_comparison.xlsx"
df.to_excel(output_file, index=False)

print("Done! Results saved to:", output_file)
print(df)