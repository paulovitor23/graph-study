"""Run the TP3 pipeline on bounded fixtures, including skip/abort policies."""

import csv
import json
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile


def read(path):
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source))


def main():
    executable, runner = map(lambda value: str(Path(value).resolve()), sys.argv[1:])
    spec = importlib.util.spec_from_file_location("tp3_runner", runner)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # 100 s fixos + mil destinos de 0,1 s: o preparo nao pode ser multiplicado por V.
    if abs(module.target_projection(101.7, 100.1, 17, 1, 100, 1, 10) - 200) > 1e-9:
        raise RuntimeError("Projecao multiplicou o custo fixo de preparacao")
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        chain = root / "chain.txt"
        chain.write_text("100\n" + "".join(f"{i} {i+1} 1\n" for i in range(1, 100)))
        negative = root / "negative.txt"
        negative.write_text("100\n10 20 1\n20 10 -2\n20 100 1\n")
        def execute(name, graphs, modes=None, limit=30, memory=256):
            manifest = root / f"{name}.json"
            manifest.write_text(json.dumps({"rounds": 10, "time_limit_s": limit,
                                           "algorithms": modes or ["bf_target", "floyd", "bf_all_pairs"], "graphs": graphs}))
            output = root / name
            command = [sys.executable, runner, str(manifest), "--executable", executable,
                       "--output", str(output), "--memory-mib", str(memory)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=30)
            if result.returncode != 0:
                raise RuntimeError(result.stdout + result.stderr)
            return output
        output = execute("complete", [{"id": "chain", "path": "chain.txt", "edges": 99},
                                       {"id": "negative", "path": "negative.txt", "edges": 3}])
        subprocess.run([sys.executable, str(Path(runner).with_name("verify_tp3.py")), str(output)], check=True,
                       capture_output=True, text=True, timeout=10)
        subprocess.run([sys.executable, str(Path(runner).with_name("summarize_tp3.py")), str(output)], check=True,
                       capture_output=True, text=True, timeout=10)
        if "90,00" not in (output / "RESPOSTAS.md").read_text(encoding="utf-8") or not (output / "ROTEIRO_8_MIN.md").exists():
            raise RuntimeError("Respostas ou roteiro nao foram gerados dos CSVs")
        q1 = read(output / "q1_bellman_ford.csv")
        if [float(row["distance"]) for row in q1 if row["graph"] == "chain"] != [90, 80, 70]:
            raise RuntimeError("Q1 incorreta")
        if [row["status"] for row in q1 if row["graph"] == "negative"] != ["negative_cycle", "negative_cycle", "unreachable"]:
            raise RuntimeError("Ciclo negativo mal classificado")
        q3 = read(output / "q3_tempos.csv")
        if len(q3) != 6 or any(row["status"] != "complete" or row["completed_rounds"] != "10"
                               or not row["official_mean_s"] for row in q3):
            raise RuntimeError("Media oficial nao corresponde a dez rodadas completas")
        for state in q3:
            samples = read(output / state["graph"] / state["algorithm"] / "timings.csv")
            mean = sum(float(row["seconds"]) for row in samples) / 10
            if abs(mean - float(state["official_mean_s"])) > 1e-9:
                raise RuntimeError("Media incorreta")
            if state["algorithm"] == "bf_all_pairs" and any(row["targets"] != "100" for row in samples):
                raise RuntimeError("Rodada de todos os pares incompleta")
        large = root / "large.txt"
        large.write_text("25000\n")
        skipped = execute("memory", [{"id": "large", "path": "large.txt", "edges": 0},
                                     {"id": "small", "path": "chain.txt", "edges": 99}], ["floyd"], memory=128)
        states = read(skipped / "q3_tempos.csv")
        if [(row["status"], row["reason"]) for row in states] != [("skipped", "memory_limit"), ("complete", "")]:
            raise RuntimeError("Nao continuou apos pular grafo inviavel")
        predicted = root / "predicted.txt"
        predicted.write_text("100000\n")
        forecast = execute("forecast", [{"id": "slow", "path": "predicted.txt", "edges": 0},
                                         {"id": "small", "path": "chain.txt", "edges": 99}],
                           ["bf_all_pairs"], limit=.5)
        forecast_rows = read(forecast / "q3_tempos.csv")
        state = forecast_rows[0]
        if state["reason"] != "predicted_time_limit" or state["official_mean_s"]:
            raise RuntimeError("Projecao longa nao foi interrompida")
        if forecast_rows[1]["status"] != "complete":
            raise RuntimeError("Nao continuou apos previsao de tempo inviavel")
        aborted = execute("timeout", [{"id": "chain", "path": "chain.txt", "edges": 99}],
                          ["bf_target"], limit=1e-9)
        state = read(aborted / "q3_tempos.csv")[0]
        if state["reason"] != "time_limit" or state["official_mean_s"]:
            raise RuntimeError("Prazo nao aplicado")
        if sys.platform.startswith("linux"):
            fake = root / "partial_worker.py"
            fake.write_text("#!/usr/bin/env python3\nimport pathlib,sys,json\n"
                "p=pathlib.Path(sys.argv[2]); p.mkdir(exist_ok=True)\n"
                "(p/'timings.csv').write_text('round,seconds,targets,checksum,has_negative_cycle,relaxation_passes,examined_edges\\n1,0.5,1,123,0,2,4\\n2,')\n"
                "print(json.dumps({'event':'done','status':'aborted','reason':'time_limit'}),flush=True)\n"
                "raise SystemExit(2)\n")
            fake.chmod(0o755)
            saved_executable = executable
            executable = str(fake)
            partial = execute("partial", [{"id": "chain", "path": "chain.txt", "edges": 99}], ["bf_target"])
            executable = saved_executable
            state = read(partial / "q3_tempos.csv")[0]
            if state["completed_rounds"] != "1" or state["official_mean_s"] or float(state["mean_s"]) != .5:
                raise RuntimeError("Rodada incompleta foi incluida ou resultado concluido foi perdido")
            fake.write_text("#!/usr/bin/env python3\nimport sys\ntry:\n data=bytearray(512*1024*1024)\n"
                            "except MemoryError:\n raise SystemExit(3)\nraise SystemExit(0)\n")
            executable = str(fake)
            limited = execute("allocation_limit", [{"id": "chain", "path": "chain.txt", "edges": 99}],
                              ["bf_target"], memory=128)
            executable = saved_executable
            state = read(limited / "q3_tempos.csv")[0]
            if state["reason"] != "memory_limit" or state["official_mean_s"]:
                raise RuntimeError("Limite de enderecamento nao foi aplicado ao processo")
        again = subprocess.run([sys.executable, runner, str(root / "complete.json"), "--executable", executable,
                                "--output", str(output)], capture_output=True, text=True, timeout=10)
        if again.returncode == 0:
            raise RuntimeError("Execucao anterior poderia ser sobrescrita")
    print("TP3 pipeline checks passed")


if __name__ == "__main__":
    main()
