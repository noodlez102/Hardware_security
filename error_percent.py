#!/usr/bin/env python3

import argparse
import random
import subprocess
import re
import sys
import time

NUM_BITS = 512

random.seed()
transmitted = ''.join(random.choice('01') for _ in range(NUM_BITS))

rx_cmd = ["./receiver", "--bits", str(NUM_BITS)]


tx_cmd = ["./transmitter", "--binary", transmitted]

print("[eval] Starting receiver...")
rx_proc = subprocess.Popen(rx_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

time.sleep(0.5)

print("[eval] Starting transmitter...")
tx_proc = subprocess.Popen(tx_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

rx_output = rx_proc.stdout.read()
rx_proc.wait()

match = re.search(r'received bits\s*->\s*"([01]+)"', rx_output)
if not match:
    print("[eval] ERROR: could not parse received bits from receiver output")
    sys.exit(1)

received = match.group(1)

if len(received) != NUM_BITS:
    print(f"[eval] WARNING: expected {NUM_BITS} bits, got {len(received)}")
    min_len = min(len(transmitted), len(received))
    transmitted = transmitted[:min_len]
    received    = received[:min_len]

# ── Compare bit by bit ────────────────────────────────────────────────────────
errors      = sum(t != r for t, r in zip(transmitted, received))
total       = len(transmitted)
correct     = total - errors
accuracy    = correct / total * 100
error_rate  = errors  / total * 100

# ── Pretty results ────────────────────────────────────────────────────────────
print("=" * 50)
print(f"  Total bits transmitted : {total}")
print(f"  Correct bits           : {correct}")
print(f"  Bit errors             : {errors}")
print(f"  Accuracy               : {accuracy:.2f}%")
print(f"  Error rate             : {error_rate:.2f}%")
print("=" * 50)
print()

# Show first 64 bits side by side for a visual check
N = min(64, total)
print(f"  First {N} bits comparison (T=transmitted, R=received):")
print(f"  T: {transmitted[:N]}")
print(f"  R: {received[:N]}")
diff = ''.join('^' if t != r else ' ' for t, r in zip(transmitted[:N], received[:N]))
print(f"     {diff}  ({diff.count('^')} errors in first {N})")
print()

# Per-bit error breakdown (print positions of all errors)
if errors > 0 and errors <= 100:
    error_positions = [i for i, (t, r) in enumerate(zip(transmitted, received)) if t != r]
    print(f"  Error positions: {error_positions}")
elif errors > 100:
    print(f"  (too many errors to list individually)")
else:
    print("  No errors detected — perfect transmission!")