"""Compare historical CLI results against a preserved build, ignoring timings."""

import argparse
import csv
from pathlib import Path
import subprocess
import tempfile


def run(command):
    subprocess.run(list(map(str, command)), check=True, capture_output=True, timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("revised", type=Path)
    args = parser.parse_args()
    builds = [args.baseline.resolve(), args.revised.resolve()]
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        graph = root / "graph.txt"
        graph.write_text("60\n" + "".join(f"{i} {i+1} {i % 5}\n" for i in range(1, 60))
                         + "1 60 100\n60 1 101\n5 5 0\n1 20 14\n")
        names = root / "names.csv"
        names.write_text("\n".join(f"{i+1},{name}" for i, name in enumerate([
            "Edsger W. Dijkstra", "Alan M. Turing", "J. B. Kruskal",
            "Jon M. Kleinberg", "Éva Tardos", "Daniel R. Figueiredo"
        ])) + "\n", encoding="utf-8")
        small = root / "small.txt"
        small.write_text("6\n1 2 2\n2 3 1\n3 4 0\n4 5 3\n5 6 1\n1 5 9\n")
        results = []
        for index, build in enumerate(builds):
            output = root / str(index)
            output.mkdir()
            result = {}
            for representation in ["list", "matrix"]:
                prefix = output / representation
                run([build / "graph_experiments", graph, representation, prefix, "3", "exact", "1"])
                for suffix in ["components.txt", "search_trees.txt"]:
                    result[f"{representation}_{suffix}"] = Path(f"{prefix}_{suffix}").read_bytes()
                summary = Path(f"{prefix}_summary.txt").read_text()
                result[f"{representation}_summary"] = "\n".join(line for line in summary.splitlines()
                    if not line.startswith(("tempo_", "memoria_", "arquivo_arvores=")))
            prefix = output / "weighted"
            run([build / "weighted_graph_experiments", graph, prefix, "3", "242", "both"])
            for suffix in ["paths.csv", "sources.csv"]:
                result[suffix] = Path(f"{prefix}_{suffix}").read_bytes()
            with Path(f"{prefix}_benchmark.csv").open() as source:
                result["benchmark_fields"] = [{k: v for k, v in row.items()
                    if k not in ["total_ms", "average_ms"]} for row in csv.DictReader(source)]
            researcher_output = output / "researchers.csv"
            run([build / "researcher_experiments", small, names, researcher_output])
            result["researchers"] = researcher_output.read_bytes()
            results.append(result)
        different = [key for key in results[0] if results[0][key] != results[1][key]]
        if different:
            raise RuntimeError(f"Resultados alterados: {different}")
        print("Todas as saídas funcionais das três CLIs coincidem com a versão preservada.")


if __name__ == "__main__":
    main()
