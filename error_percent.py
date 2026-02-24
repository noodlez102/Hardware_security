#!/usr/bin/env python3
"""
evaluate.py - Measure the error rate of the covert channel.

Transmits at least 512 random bits, waits for the receiver to finish,
then compares input vs output and reports accuracy / error rate.

Usage:
    python3 evaluate.py [--bits 512] [--threshold T]

Both ./transmitter and ./receiver must be in the current directory,
as well as ./simple_stream (and ./run_multiple.sh if used).
"""

import argparse
import random
import subprocess
import re
import sys
import time

# ── Argument parsing ─────────────────────────────────────────────────────────
parser = argparse.ArgumentParser(description="Covert channel error rate evaluator")
parser.add_argument("--bits",      type=int,   default=512, help="Number of bits to transmit (>=512)")
parser.add_argument("--threshold", type=float, default=0.0, help="Fixed BW threshold for receiver (0=auto)")
args = parser.parse_args()

NUM_BITS = max(512, args.bits)

# ── Generate random bit string ────────────────────────────────────────────────
random.seed()
transmitted = ''.join(random.choice('01') for _ in range(NUM_BITS))
print(f"[eval] Transmitting {NUM_BITS} bits")
print(f"[eval] First 64 bits: {transmitted[:64]}")
print()

# ── Build receiver command ────────────────────────────────────────────────────
rx_cmd = ["./receiver", "--bits", str(NUM_BITS)]
if args.threshold > 0:
    rx_cmd += ["--threshold", str(args.threshold)]

tx_cmd = ["./transmitter", "--binary", transmitted]

# ── Launch receiver first, then transmitter ───────────────────────────────────
print("[eval] Starting receiver...")
rx_proc = subprocess.Popen(rx_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

# Small delay so receiver can calibrate and write sync file before transmitter starts
time.sleep(0.5)

print("[eval] Starting transmitter...")
tx_proc = subprocess.Popen(tx_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

# ── Stream transmitter output live ───────────────────────────────────────────
print()
for line in tx_proc.stdout:
    print(f"  [tx] {line}", end="")
tx_proc.wait()
print()

# ── Collect receiver output ───────────────────────────────────────────────────
rx_output = rx_proc.stdout.read()
rx_proc.wait()

# Print receiver output
for line in rx_output.splitlines():
    print(f"  [rx] {line}")
print()

# ── Parse received bit string from receiver output ────────────────────────────
# Looks for the line: receiver: received bits -> "0100110..."
match = re.search(r'received bits\s*->\s*"([01]+)"', rx_output)
if not match:
    print("[eval] ERROR: could not parse received bits from receiver output")
    sys.exit(1)

received = match.group(1)

if len(received) != NUM_BITS:
    print(f"[eval] WARNING: expected {NUM_BITS} bits, got {len(received)}")
    # Truncate or pad to compare what we can
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