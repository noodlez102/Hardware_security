#!/usr/bin/env python3

import argparse
import random
import subprocess
import re
import sys
import time
import math

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

start_time = math.floor(time.time() / 60.0) * 60.0 + 60.0 
rx_output = rx_proc.stdout.read()
rx_proc.wait()

elapsed = time.time() - start_time

match = re.search(r'received bits\s*->\s*"([01]+)"', rx_output)
if not match:
    print("[eval] ERROR: could not parse received bits from receiver output")
    sys.exit(1)

received = match.group(1)

errors      = sum(t != r for t, r in zip(transmitted, received))
total       = len(transmitted)
correct     = total - errors
accuracy    = correct / total * 100
error_rate  = errors  / total * 100
bandwidth   = total / elapsed         
goodput     = correct / elapsed
print("=" * 50)
print(f"  Total bits transmitted : {total}")
print(f"  Correct bits           : {correct}")
print(f"  Bit errors             : {errors}")
print(f"  Accuracy               : {accuracy:.2f}%")
print(f"  Error rate             : {error_rate:.2f}%")
print(f"  Elapsed time           : {elapsed:.2f}s")
print(f"  Raw bandwidth          : {bandwidth:.4f} bits/sec")
print(f"  Goodput (correct only) : {goodput:.4f} bits/sec")
print("=" * 50)
print()
