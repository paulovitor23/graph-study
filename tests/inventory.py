"""Check inventory semantics without loading a large graph."""

import csv
from pathlib import Path
import subprocess
import sys
import tempfile


with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "graph.txt"
    path.write_text("4\n1 2 1\n1 2 3\n2 1 -2\n3 3 0\n0 2 1\n1 5 1\n1\n")
    result = subprocess.run([sys.argv[1], str(path)], capture_output=True, text=True, timeout=10)
    rows = list(csv.DictReader(result.stdout.splitlines()))
    expected = {"vertices": "4", "edge_lines": "7", "unique_ordered_arcs": "3",
                "ordered_duplicates": "1", "self_loops": "1", "negative_weight_lines": "1",
                "min_weight": "-2", "max_weight": "3", "invalid_ids": "2", "malformed_lines": "1"}
    if result.returncode != 1 or rows != [expected]:
        raise RuntimeError(f"inventory mismatch: {result.returncode}, {rows}, {result.stderr}")
print("Inventory checks passed")
