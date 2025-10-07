#!/usr/bin/env python3
"""
Binary reader for the tree format used by the project.

File format (stream of records):
- Leaf node: int32 (label), float64 (sink capacitance)
- Non-leaf node: int32 (value -1), float64 (wire length to left child),
  float64 (wire length to right child), int32 (number of parallel inverters)

If a non-leaf node has only a branch, the right-child wire length is -1.0.

This script reads the file and prints each parsed record in preorder order.
"""

import struct
import sys
from pathlib import Path


def read_tree_bin_file(path):
    """Read the tree binary file and yield records as dicts.

    Yields:
        dict: for a leaf: {'type': 'leaf', 'label': int, 'cap': float}
              for an internal node: {'type': 'internal', 'left_len': float, 'right_len': float, 'num_inv': int}
    """
    with open(path, 'rb') as f:
        while True:
            int_bytes = f.read(4)
            if not int_bytes:
                break
            if len(int_bytes) < 4:
                raise EOFError('Unexpected end of file while reading record header (int32)')
            (tag,) = struct.unpack('=i', int_bytes)
            if tag == -1:
                # internal node: read two doubles and an int
                rest = f.read(8 + 8 + 4)
                if len(rest) < 20:
                    raise EOFError('Unexpected end of file while reading internal node')
                left_len, right_len, num_inv = struct.unpack('=ddi', rest)
                yield {
                    'type': 'internal',
                    'left_len': left_len,
                    'right_len': right_len,
                    'num_inv': num_inv,
                }
            else:
                # leaf: tag is the label, then a double for capacitance
                cap_bytes = f.read(8)
                if len(cap_bytes) < 8:
                    raise EOFError('Unexpected end of file while reading leaf capacitance')
                (cap,) = struct.unpack('=d', cap_bytes)
                yield {
                    'type': 'leaf',
                    'label': tag,
                    'cap': cap,
                }


def main(argv):
    if len(argv) >= 2 and argv[1] in ('-h', '--help'):
        print(f"Usage: {Path(argv[0]).name} [file]")
        print('Prints a readable listing of the binary tree file format.')
        return 0

    file_path = argv[1] if len(argv) >= 2 else 'example/5.btopo'
    p = Path(file_path)
    if not p.exists():
        print(f'Error: file not found: {file_path}', file=sys.stderr)
        return 2

    try:
        for rec in read_tree_bin_file(p):
            if rec['type'] == 'leaf':
                # match textual representation used elsewhere: label(cap)
                print(f"{rec['label']}({rec['cap']:.10e})")
            else:
                # internal: (left right num_inv)
                print(f"({rec['left_len']:.10e} {rec['right_len']:.10e} {rec['num_inv']})")
    except EOFError as e:
        print('Error: malformed file -', e, file=sys.stderr)
        return 3

    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv))