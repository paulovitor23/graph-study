"""Run isolated profiles with a deadline; keep raw samples and environment."""

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import time


def sha256(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("graph", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--rounds", type=int, default=5)
    parser.add_argument("--sources", type=int, default=3)
    parser.add_argument("--mode", choices=["load", "search", "all", "matrix"], default="search")
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--baseline", type=Path, help="alternate candidate/baseline order and compare result signatures")
    args = parser.parse_args()
    if min(args.rounds, args.sources, args.timeout) <= 0:
        parser.error("rounds, sources and timeout must be positive")
    command = [str(args.executable.resolve()), str(args.graph.resolve()), str(args.sources), args.mode]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    metadata = {
        "command": command, "rounds": args.rounds, "timeout_s": args.timeout,
        "platform": platform.platform(), "machine": platform.machine(),
        "cpu_count": os.cpu_count(), "graph_sha256": sha256(args.graph),
        "executable_sha256": sha256(args.executable),
        "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "memory_scope": "process RSS and process high-water RSS; bytes; zero if unsupported",
        "timing_scope": "algorithm call including result initialization; excludes I/O, hashing and result destruction",
        "warmup": "none; separate processes; rounds may share OS file cache",
        "status": "running",
    }
    for name in ["cpuinfo", "meminfo"]:
        path = Path("/proc") / name
        if path.exists():
            metadata[name] = path.read_text()
    metadata_path = args.output.with_suffix(".json")
    if args.baseline:
        metadata["baseline_executable_sha256"] = sha256(args.baseline)
        metadata["baseline_executable"] = str(args.baseline.resolve())
        metadata["order"] = "baseline/candidate on odd rounds; candidate/baseline on even rounds"
    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    with args.output.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=["version", "round", "metric", "ms", "resident_bytes", "peak_bytes", "digest"])
        writer.writeheader()
        expected = None
        for round_number in range(1, args.rounds + 1):
            candidates = [("candidate", args.executable)]
            if args.baseline:
                candidates.insert(0, ("baseline", args.baseline))
                if round_number % 2 == 0:
                    candidates.reverse()
            try:
                for version, executable in candidates:
                    current_command = [str(executable.resolve()), *command[1:]]
                    result = subprocess.run(current_command, capture_output=True, text=True, timeout=args.timeout, check=True)
                    samples = list(csv.DictReader(result.stdout.splitlines()))
                    signature = [(sample["metric"], sample["digest"]) for sample in samples]
                    if not samples or (expected is not None and signature != expected):
                        raise RuntimeError("resultados diferentes entre versoes/rodadas")
                    expected = signature
                    for sample in samples:
                        writer.writerow({"version": version, "round": round_number, **sample})
                    output.flush()
                    print(f"{args.output.name}: {version}, rodada {round_number}/{args.rounds}", flush=True)
            except (subprocess.SubprocessError, RuntimeError) as error:
                metadata.update(status="aborted", failed_round=round_number, reason=str(error))
                metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
                raise
    metadata["status"] = "complete"
    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
