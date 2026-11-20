"""Execute the TP3 studies sequentially with memory/time admission and durable results."""

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import statistics
import struct
import subprocess
import sys
import time

MODES = ("bf_target", "floyd", "bf_all_pairs")
PAIR_VERTICES = (10, 20, 30, 40, 50)


def save_json(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    temporary.replace(path)


def read_csv(path):
    if not path.exists():
        return []
    with path.open(encoding="utf-8", newline="") as source:
        return [row for row in csv.DictReader(source) if None not in row.values()]


def write_csv(path, fields, rows):
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def sha256(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def available_memory():
    if sys.platform.startswith("linux"):
        values = dict(line.split(":", 1) for line in Path("/proc/meminfo").read_text().splitlines())
        available = int(values["MemAvailable"].split()[0]) * 1024
        limit = Path("/sys/fs/cgroup/memory.max")
        current = Path("/sys/fs/cgroup/memory.current")
        if limit.exists() and current.exists() and limit.read_text().strip() != "max":
            available = min(available, max(0, int(limit.read_text()) - int(current.read_text())))
        return available
    if sys.platform == "win32":
        import ctypes
        class MemoryStatus(ctypes.Structure):
            _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
                (name, ctypes.c_ulonglong) for name in ["total", "available", "total_page", "available_page",
                                                       "total_virtual", "available_virtual", "extended"]]
        status = MemoryStatus()
        status.length = ctypes.sizeof(status)
        if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
            raise OSError("Nao foi possivel consultar a memoria disponivel")
        return status.available
    raise RuntimeError("Informe --memory-mib nesta plataforma")


def process_rss(pid):
    if sys.platform.startswith("linux"):
        try:
            for line in Path(f"/proc/{pid}/status").read_text().splitlines():
                if line.startswith("VmRSS:"):
                    return int(line.split()[1]) * 1024
        except (FileNotFoundError, ProcessLookupError):
            pass
    return 0


def memory_estimate(vertices, edges, mode):
    # Capacidade geometrica das duas listas e espaco linear de resultados/fronteiras.
    graph = 32 * edges + 32 * vertices + 64 * 1024**2
    if mode == "floyd":
        index_bytes = struct.calcsize("P")
        buffers = vertices**2 * (8 + index_bytes) + vertices * index_bytes
        return graph + buffers, buffers
    return 64 * edges + 160 * vertices + 64 * 1024**2, 0


def target_projection(elapsed, anchor_elapsed, completed, anchor_completed, total, round_number, rounds):
    # Preparacao/pilotos entram uma vez; a taxa usa somente destinos apos a ancora.
    rate = (elapsed - anchor_elapsed) / (completed - anchor_completed)
    remaining = (rounds - round_number) * total + total - completed
    return elapsed + rate * remaining


def launch(executable, graph, mode, directory, rounds, budget, wall_seconds, seed):
    directory.mkdir(parents=True, exist_ok=True)
    required, matrix = memory_estimate(graph["vertices"], graph["edges"], mode)
    state = {"graph": graph["id"], "algorithm": mode, "status": "pending", "reason": "",
             "requested_rounds": rounds, "completed_rounds": 0, "memory_budget_bytes": budget,
             "memory_estimate_bytes": required, "matrix_bytes": matrix, "time_limit_s": wall_seconds,
             "estimated_total_s": None, "peak_sampled_rss_bytes": 0, "elapsed_wall_s": 0}
    if required > budget:
        state.update(status="skipped", reason="memory_limit")
        save_json(directory / "state.json", state)
        print(f"{graph['id']} / {mode}: pulado por memoria ({required} > {budget} bytes)", flush=True)
        return state
    command = [str(executable), graph["path"], str(directory), mode, str(rounds), str(budget),
               str(wall_seconds), str(seed)]
    state.update(command=command, started_utc=datetime.now(timezone.utc).isoformat(), status="running")
    save_json(directory / "state.json", state)
    def set_memory_limit():
        import resource
        resource.setrlimit(resource.RLIMIT_AS, (budget, budget))
    start = time.monotonic()
    terminal = None
    target_anchor = None
    def consume_event(event):
        nonlocal terminal, target_anchor
        kind = event["event"]
        if kind == "loaded":
            if event["vertices"] != graph["vertices"] or event["edges"] != graph["edges"]:
                raise RuntimeError("Inventario diverge do grafo carregado")
            state["load_s"] = event["seconds"]
        elif kind in ("progress", "forecast"):
            if event["estimated_total_s"] > 0:
                state["estimated_total_s"] = event["estimated_total_s"]
                state["estimate_method"] = event.get("method", "pass_extrapolation_assuming_no_early_stop")
            state["last_progress"] = event
        elif kind == "round":
            state["completed_rounds"] = event["round"]
            print(f"{graph['id']} / {mode}: rodada {event['round']}/{rounds}, {event['seconds']:.6f} s", flush=True)
        elif kind == "target_progress":
            elapsed = time.monotonic() - start
            if target_anchor is None or target_anchor[0] != event["round"]:
                target_anchor = (event["round"], elapsed, event["completed"])
            elif event["completed"] - target_anchor[2] >= 16:
                projection = target_projection(elapsed, target_anchor[1], event["completed"], target_anchor[2],
                                               event["total"], event["round"], rounds)
                if projection >= wall_seconds:
                    terminal = ("aborted", "predicted_time_limit")
                    state["estimated_total_s"] = projection
                    state["estimate_method"] = "observed_target_progress_wall_time"
        elif kind == "done":
            terminal = (event["status"], event.get("reason", ""))
    print(f"{graph['id']} / {mode}: iniciando pilotos e ate {rounds} rodadas", flush=True)
    with (directory / "events.jsonl").open("w", encoding="utf-8") as stdout, \
         (directory / "stderr.log").open("w", encoding="utf-8") as stderr, \
         (directory / "events.jsonl").open("r", encoding="utf-8") as events:
        process = subprocess.Popen(command, stdout=stdout, stderr=stderr,
                                   preexec_fn=set_memory_limit if sys.platform.startswith("linux") else None)
        partial = ""
        try:
            while True:
                elapsed = time.monotonic() - start
                state["elapsed_wall_s"] = elapsed
                rss = process_rss(process.pid)
                state["peak_sampled_rss_bytes"] = max(state["peak_sampled_rss_bytes"], rss)
                partial += events.read()
                lines = partial.split("\n")
                partial = lines.pop()
                for line in lines:
                    consume_event(json.loads(line))
                if terminal and terminal[0] != "complete" and process.poll() is None:
                    process.kill()
                if elapsed >= wall_seconds and process.poll() is None:
                    terminal = ("aborted", "time_limit")
                    process.kill()
                if rss > budget and process.poll() is None:
                    terminal = ("aborted", "memory_limit")
                    process.kill()
                code = process.poll()
                if code is not None:
                    # O produtor ja terminou: consumir os eventos finais antes de fechar.
                    tail = partial + events.read()
                    for line in tail.splitlines():
                        consume_event(json.loads(line))
                    state["returncode"] = code
                    break
                save_json(directory / "state.json", state)
                time.sleep(.1)
        except BaseException:
            if process.poll() is None:
                process.kill()
            process.wait()
            state.update(status="error", reason="supervisor_error", elapsed_wall_s=time.monotonic() - start)
            save_json(directory / "state.json", state)
            raise
        process.wait()
    samples = read_csv(directory / "timings.csv")
    state["completed_rounds"] = len(samples)
    state["elapsed_wall_s"] = time.monotonic() - start
    if terminal is None:
        terminal = ("aborted", "memory_limit") if state.get("returncode") == 3 else ("error", "worker_error")
    if terminal[0] == "complete" and (state.get("returncode") != 0 or len(samples) != rounds):
        terminal = ("error", "incomplete_rounds")
    state.update(status=terminal[0], reason=terminal[1])
    if samples:
        values = [float(row["seconds"]) for row in samples]
        if any(not math.isfinite(value) or value <= 0 for value in values):
            raise RuntimeError("Amostra de tempo invalida")
        state.update(mean_s=statistics.mean(values), median_s=statistics.median(values),
                     stdev_s=statistics.stdev(values) if len(values) > 1 else None,
                     minimum_s=min(values), maximum_s=max(values))
    state["official_mean_s"] = state.get("mean_s") if rounds == 10 and len(samples) == 10 and state["status"] == "complete" else None
    save_json(directory / "state.json", state)
    print(f"{graph['id']} / {mode}: {state['status']} {state['reason']}", flush=True)
    return state


def consolidate(output, graphs, states):
    fields = ["graph", "algorithm", "status", "reason", "requested_rounds", "completed_rounds", "official_mean_s",
              "mean_s", "median_s", "stdev_s", "minimum_s", "maximum_s", "estimated_total_s", "estimate_method",
              "memory_estimate_bytes", "memory_budget_bytes", "matrix_bytes", "peak_sampled_rss_bytes", "elapsed_wall_s"]
    write_csv(output / "q3_tempos.csv", fields, [{key: state.get(key, "") for key in fields} for state in states])
    q1, q2, comparisons = [], [], []
    for graph in graphs:
        by_mode = {state["algorithm"]: state for state in states if state["graph"] == graph["id"]}
        for mode, sources, targets, table in [("bf_target", (10, 20, 30), (100,), q1),
                                               ("floyd", PAIR_VERTICES, PAIR_VERTICES, q2)]:
            records = read_csv(output / graph["id"] / mode / "queries.csv")
            lookup = {}
            for row in records:
                key = (int(row["source"]), int(row["target"]))
                if key in lookup:
                    previous = lookup[key]
                    if any(previous[field] != row[field] for field in ("status", "distance", "path", "has_negative_cycle")):
                        raise RuntimeError("Consultas divergem entre pilotos e rodadas")
                lookup[key] = row
            state = by_mode.get(mode, {})
            for source in sources:
                for target in targets:
                    row = lookup.get((source, target))
                    table.append({"graph": graph["id"], "source": source, "target": target,
                                  "status": row["status"] if row else "not_computed",
                                  "distance": row["distance"] if row else "",
                                  "path": row["path"] if row else "",
                                  "has_negative_cycle": row["has_negative_cycle"] if row else "",
                                  "evidence": row["phase"] if row else "",
                                  "reason": "" if row else state.get("reason", "not_requested")})
        bf, fw = by_mode.get("bf_all_pairs", {}), by_mode.get("floyd", {})
        ratio = ""
        conclusion = "comparacao_nao_concluida"
        if bf.get("official_mean_s") is not None and fw.get("official_mean_s") is not None:
            ratio = bf["official_mean_s"] / fw["official_mean_s"]
            conclusion = "floyd_mais_rapido" if ratio > 1 else "bellman_ford_mais_rapido" if ratio < 1 else "empate"
        comparisons.append({"graph": graph["id"], "bf_status": bf.get("status", "pending"),
                            "bf_reason": bf.get("reason", ""), "bf_completed_rounds": bf.get("completed_rounds", 0),
                            "bf_mean_s": bf.get("official_mean_s"), "fw_status": fw.get("status", "pending"),
                            "fw_reason": fw.get("reason", ""), "fw_completed_rounds": fw.get("completed_rounds", 0),
                            "fw_mean_s": fw.get("official_mean_s"), "bf_over_fw": ratio, "conclusion": conclusion})
        # Nas entradas pequenas, conferir os mesmos 25 pares entre as duas abordagens.
        a = read_csv(output / graph["id"] / "bf_all_pairs" / "queries.csv")
        b = read_csv(output / graph["id"] / "floyd" / "queries.csv")
        if a and b:
            lookup = {(row["source"], row["target"]): row for row in b}
            for row in a:
                other = lookup[row["source"], row["target"]]
                if row["status"] != other["status"]:
                    raise RuntimeError("BF todos os pares e FW discordam no estado")
                if row["status"] == "finite" and not math.isclose(float(row["distance"]), float(other["distance"]), rel_tol=1e-9, abs_tol=1e-9):
                    raise RuntimeError("BF todos os pares e FW discordam na distancia")
    query_fields = ["graph", "source", "target", "status", "distance", "path", "has_negative_cycle", "evidence", "reason"]
    write_csv(output / "q1_bellman_ford.csv", query_fields, q1)
    write_csv(output / "q2_floyd_warshall.csv", query_fields, q2)
    write_csv(output / "q4_comparacao.csv", list(comparisons[0]) if comparisons else [], comparisons)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--memory-mib", type=float)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    rounds = manifest.get("rounds", 10)
    wall_seconds = manifest.get("time_limit_s", 7200)
    modes = manifest.get("algorithms", list(MODES))
    seed = manifest.get("seed", 242)
    if isinstance(rounds, bool) or not isinstance(rounds, int) or rounds <= 0 or \
       not isinstance(wall_seconds, (int, float)) or not math.isfinite(wall_seconds) or wall_seconds <= 0 or \
       set(modes) - set(MODES) or len(set(modes)) != len(modes) or not modes or \
       isinstance(seed, bool) or not isinstance(seed, int) or not 0 <= seed < 2**64:
        parser.error("Manifesto invalido")
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error("A pasta de saida deve estar vazia para preservar execucoes anteriores")
    output.mkdir(parents=True, exist_ok=True)
    executable = args.executable.resolve(strict=True)
    graphs = []
    for item in manifest["graphs"]:
        if not re.fullmatch(r"[A-Za-z0-9_-]+", item["id"]) or any(graph["id"] == item["id"] for graph in graphs):
            parser.error("Identificador de grafo invalido ou repetido")
        path = (args.manifest.parent / item["path"]).resolve(strict=True)
        with path.open() as source:
            header = source.readline().strip()
        if not header.isdecimal() or int(header) < 100 or int(item["edges"]) < 0:
            parser.error("Grafo invalido para as consultas fixas do TP3")
        actual_hash = sha256(path)
        if item.get("sha256") and actual_hash != item["sha256"]:
            parser.error(f"Hash divergente: {path}")
        graphs.append({**item, "path": str(path), "vertices": int(header), "bytes": path.stat().st_size, "sha256": actual_hash})
    if not graphs:
        parser.error("Nenhum grafo informado")
    if args.memory_mib is not None and (not math.isfinite(args.memory_mib) or args.memory_mib <= 0):
        parser.error("Orcamento de memoria invalido")
    environment = {"started_utc": datetime.now(timezone.utc).isoformat(), "platform": platform.platform(),
                   "python": sys.version, "executable": str(executable), "executable_sha256": sha256(executable),
                   "direction": "directed", "official_rounds": 10, "configured_rounds": rounds,
                   "clock": "std::chrono::steady_clock", "time_unit": "seconds",
                   "timing_scope": "algorithm calls and all-target loop; excludes input/output, validation, result destruction and pilots",
                   "cache": "same graph in memory; new algorithm result per round; no CPU affinity or frequency control",
                   "memory_enforcement": "RLIMIT_AS plus sampled RSS" if sys.platform.startswith("linux") else "preflight estimate and process timeout",
                   "manifest": {**manifest, "graphs": graphs}}
    for name in ("cpuinfo", "meminfo"):
        path = Path("/proc") / name
        if path.exists():
            environment[name] = path.read_text()
    save_json(output / "environment.json", environment)
    states = []
    for index, graph in enumerate(graphs):
        order = modes if index % 2 == 0 else list(reversed(modes))
        for mode in order:
            budget = int(args.memory_mib * 1024**2) if args.memory_mib is not None else int(available_memory() * .7)
            state = launch(executable, graph, mode, output / graph["id"] / mode, rounds, budget, wall_seconds, seed)
            states.append(state)
            save_json(output / "states.json", states)
            consolidate(output, graphs, states)
    environment["finished_utc"] = datetime.now(timezone.utc).isoformat()
    environment["status"] = "complete_with_reported_limits" if any(state["status"] != "complete" for state in states) else "complete"
    save_json(output / "environment.json", environment)
    if any(state["status"] == "error" for state in states):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
