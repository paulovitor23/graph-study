"""Summarize raw profile samples without mixing versions or algorithms."""

import argparse
from collections import defaultdict
import csv
from pathlib import Path
import statistics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", type=Path, nargs="+")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fields = ["dataset", "version", "metric", "samples", "mean_ms", "median_ms", "stdev_ms",
              "minimum_ms", "maximum_ms", "median_resident_bytes", "maximum_peak_bytes"]
    with args.output.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        for path in args.inputs:
            groups = defaultdict(list)
            with path.open(newline="", encoding="utf-8") as source:
                for row in csv.DictReader(source):
                    groups[row["version"], row["metric"]].append(row)
            for (version, metric), rows in groups.items():
                times = [float(row["ms"]) for row in rows]
                writer.writerow({
                    "dataset": path.stem, "version": version, "metric": metric, "samples": len(rows),
                    "mean_ms": statistics.mean(times), "median_ms": statistics.median(times),
                    "stdev_ms": statistics.stdev(times) if len(times) > 1 else "",
                    "minimum_ms": min(times), "maximum_ms": max(times),
                    "median_resident_bytes": statistics.median(int(row["resident_bytes"]) for row in rows),
                    "maximum_peak_bytes": max(int(row["peak_bytes"]) for row in rows),
                })


if __name__ == "__main__":
    main()
