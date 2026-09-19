import re
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} input.c output.bin")
    sys.exit(1)

input_file = Path(sys.argv[1])
output_file = Path(sys.argv[2])

text = input_file.read_text()

# Find the first uint8_t array containing the image data
match = re.search(
    r'uint8_t\s+\w+\s*\[\s*\]\s*=\s*\{(.*?)\};',
    text,
    re.DOTALL
)

if not match:
    print("ERROR: Could not find uint8_t image array")
    sys.exit(1)

array_text = match.group(1)

# Extract hexadecimal byte values
values = re.findall(
    r'0[xX]([0-9a-fA-F]{1,2})',
    array_text
)

data = bytes(int(x, 16) for x in values)

expected_size = 200 * 200 * 3

print(f"Input : {input_file}")
print(f"Bytes found : {len(data)}")
print(f"Expected    : {expected_size}")

if len(data) != expected_size:
    print("ERROR: Byte count does not match 177 x 180 RGB565A8")
    sys.exit(1)

output_file.write_bytes(data)

print(f"Output: {output_file}")
print(f"Size  : {len(data)} bytes")
print("CONVERSION SUCCESS")
