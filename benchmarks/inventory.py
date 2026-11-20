"""Inventory graph files sequentially, recording hashes and bounded execution."""

import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("graphs", type=Path, nargs="+")
    parser.add_argument("--timeout", type=float, default=180)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("timeout must be positive")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fields = ["graph", "bytes", "sha256", "status", "reason", "vertices", "edge_lines",
              "unique_ordered_arcs", "ordered_duplicates", "self_loops", "negative_weight_lines",
              "min_weight", "max_weight", "invalid_ids", "malformed_lines"]
    with args.output.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        for graph in args.graphs:
            row = {"graph": graph.name, "bytes": graph.stat().st_size}
            with graph.open("rb") as source:
                row["sha256"] = hashlib.file_digest(source, "sha256").hexdigest()
            try:
                command = [str(args.executable.resolve()), str(graph.resolve())]
                result = subprocess.run(command, text=True, capture_output=True, timeout=args.timeout)
                records = list(csv.DictReader(result.stdout.splitlines()))
                if len(records) != 1:
                    raise RuntimeError(result.stderr or "invalid inventory output")
                row.update(records[0], status="complete" if result.returncode == 0 else "invalid")
            except subprocess.TimeoutExpired:
                row.update(status="aborted", reason="time_limit")
            except RuntimeError as error:
                row.update(status="error", reason=str(error))
            writer.writerow(row)
            output.flush()
            print(graph.name, row["status"], flush=True)
    with args.executable.open("rb") as source:
        binary_hash = hashlib.file_digest(source, "sha256").hexdigest()
    args.output.with_suffix(".json").write_text(json.dumps({
        "command": [str(args.executable.resolve()), "<graph>"], "timeout_s_per_graph": args.timeout,
        "executable_sha256": binary_hash, "duplicates": "ordered pairs; reverse arcs are distinct",
        "weights": "statistics count input lines before duplicate reduction"
    }, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
