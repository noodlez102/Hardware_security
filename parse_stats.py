import os
import re
import pandas as pd

base_dir = "."  # current directory

data = []

# Loop through everything in current directory
for item in os.listdir(base_dir):
    folder_path = os.path.join(base_dir, item)

    # Only process directories
    if not os.path.isdir(folder_path):
        continue

    stats_path = os.path.join(folder_path, "stats.txt")

    # Skip if no stats.txt
    if not os.path.exists(stats_path):
        continue

    with open(stats_path, "r") as f:
        text = f.read()

    # Extract total cells (area proxy)
    cells_match = re.search(r"Number of cells:\s+(\d+)", text)
    cells = int(cells_match.group(1)) if cells_match else None

    # Extract timing
    timing_match = re.search(r"Timing estimate:\s+([\d\.]+)\s+ns\s+\(([\d\.]+)\s+MHz\)", text)
    delay = float(timing_match.group(1)) if timing_match else None
    freq = float(timing_match.group(2)) if timing_match else None

    # Extract some cell types (optional but useful)
    lut_match = re.search(r"SB_LUT4\s+(\d+)", text)
    dff_match = re.search(r"SB_DFF\s+(\d+)", text)

    lut = int(lut_match.group(1)) if lut_match else 0
    dff = int(dff_match.group(1)) if dff_match else 0

    data.append({
        "Folder": item,
        "Total Cells": cells,
        "LUT4 Count": lut,
        "DFF Count": dff,
        "Delay (ns)": delay,
        "Freq (MHz)": freq
    })

# Convert to DataFrame
df = pd.DataFrame(data)

# Sort for readability
df = df.sort_values(by="Folder")

# Save to Excel
output_file = "netlist_stats.xlsx"
df.to_excel(output_file, index=False)

print("Done! Saved to", output_file)
print(df)