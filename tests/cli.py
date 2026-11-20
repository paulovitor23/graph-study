"""Small end-to-end checks for the historical experiment commands."""

import csv
from pathlib import Path
import subprocess
import sys
import tempfile


def run(command, success=True):
    result = subprocess.run(command, capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(f"unexpected exit {result.returncode}: {command}\n{result.stderr}")


def check_io(prefix):
    with Path(f"{prefix}_io.csv").open() as source:
        rows = list(csv.DictReader(source))
    if len(rows) != 1 or any(float(rows[0][key]) < 0 for key in ["load_ms", "export_ms"]):
        raise RuntimeError("invalid phase timings")


def main():
    unweighted, weighted, researchers = map(str, map(Path, sys.argv[1:]))
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        graph = root / "graph.txt"
        graph.write_text("60\n" + "".join(f"{i} {i+1} 1\n" for i in range(1, 60)))
        for representation in ["list", "matrix"]:
            prefix = root / representation / "run"
            run([unweighted, str(graph), representation, str(prefix), "3", "exact", "1"])
            summary = Path(f"{prefix}_summary.txt").read_text()
            check_io(prefix)
            if "arestas=59" not in summary or "valor=59" not in summary:
                raise RuntimeError("incorrect graph summary")
            for suffix in ["components.txt", "search_trees.txt"]:
                if not Path(f"{prefix}_{suffix}").is_file():
                    raise RuntimeError(f"missing {suffix}")
        for value in ["-1", "0", "18446744073709551616", "2x"]:
            run([unweighted, str(graph), "list", str(root / "invalid"), value], False)
            run([weighted, str(graph), str(root / "invalid"), value], False)
        run([weighted, str(graph), str(root / "invalid"), "2", "-1"], False)
        prefix = root / "weighted" / "run"
        run([weighted, str(graph), str(prefix), "3", "0", "both"])
        check_io(prefix)
        with Path(f"{prefix}_paths.csv").open() as source:
            paths = list(csv.DictReader(source))
        if [float(row["distance"]) for row in paths] != [10, 20, 30, 40, 50]:
            raise RuntimeError("incorrect paths")
        repeat = root / "repeat"
        run([weighted, str(graph), str(repeat), "3", "242", "both", f"{prefix}_sources.csv"])
        if Path(f"{prefix}_paths.csv").read_bytes() != Path(f"{repeat}_paths.csv").read_bytes():
            raise RuntimeError("source replay changed paths")
        invalid_sources = root / "invalid_sources.csv"
        for contents in [
            "order,source\n1,-1\n2,2\n3,3\n",
            "order,source\n1,1\n2,1\n3,3\n",
            "order,source\n1,1\n3,2\n2,3\n",
            "order,source\n1,1,extra\n2,2\n3,3\n",
            "order,source\n1,1\n2,2\n",
            "wrong_header\n1,1\n2,2\n3,3\n",
        ]:
            invalid_sources.write_text(contents)
            run([weighted, str(graph), str(repeat), "3", "242", "both", str(invalid_sources)], False)
        names = root / "names.csv"
        names.write_text("\n".join(f"{i+1},{name}" for i, name in enumerate([
            "Edsger W. Dijkstra", "Alan M. Turing", "J. B. Kruskal",
            "Jon M. Kleinberg", "Éva Tardos", "Daniel R. Figueiredo"
        ])) + "\n", encoding="utf-8")
        small = root / "small.txt"
        small.write_text("6\n1 2 1\n2 3 1\n3 4 1\n4 5 1\n5 6 1\n")
        output = root / "researchers" / "results.csv"
        run([researchers, str(small), str(names), str(output)])
        check_io(output)
        with output.open(encoding="utf-8") as source:
            rows = list(csv.DictReader(source))
        if [float(row["distance"]) for row in rows] != [1, 2, 3, 4, 5]:
            raise RuntimeError("incorrect researcher distances")
        if sys.platform.startswith("linux"):
            # /dev/full abre normalmente, mas a escrita deve falhar no flush.
            (root / "full_paths.csv").symlink_to("/dev/full")
            run([weighted, str(graph), str(root / "full"), "1", "242", "heap"], False)
    print("Experiment CLI checks passed")


if __name__ == "__main__":
    main()
