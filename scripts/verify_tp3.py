"""Verify completed rounds, official means, query states and the four TP3 tables."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import statistics

from run_tp3 import read_csv, save_json


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def verify(directory):
    environment = json.loads((directory / "environment.json").read_text(encoding="utf-8"))
    require(bool(environment.get("finished_utc")), "Execucao ainda nao terminou")
    manifest = environment["manifest"]
    graphs = {graph["id"]: graph for graph in manifest["graphs"]}
    expected_jobs = {(graph, mode) for graph in graphs for mode in manifest["algorithms"]}
    states = json.loads((directory / "states.json").read_text(encoding="utf-8"))
    require({(s["graph"], s["algorithm"]) for s in states} == expected_jobs, "Configuracoes ausentes ou inesperadas")
    require(len(states) == len(expected_jobs), "Configuracao duplicada")
    summary = {(row["graph"], row["algorithm"]): row for row in read_csv(directory / "q3_tempos.csv")}
    for state in states:
        name = f"{state['graph']} / {state['algorithm']}"
        require(state["status"] in ("complete", "skipped", "aborted"), f"Erro nao resolvido: {name}")
        samples = read_csv(directory / state["graph"] / state["algorithm"] / "timings.csv")
        require(len(samples) == state["completed_rounds"], f"Contagem de rodadas divergente: {name}")
        require([int(row["round"]) for row in samples] == list(range(1, len(samples) + 1)), f"Ordem de rodadas invalida: {name}")
        require(len({row["checksum"] for row in samples}) <= 1, f"Assinaturas inconsistentes: {name}")
        targets = 1 if state["algorithm"] == "bf_target" else graphs[state["graph"]]["vertices"]
        require(all(int(row["targets"]) == targets for row in samples), f"Rodada nao percorreu todos os destinos: {name}")
        values = [float(row["seconds"]) for row in samples]
        require(all(math.isfinite(value) and value > 0 for value in values), f"Tempo invalido: {name}")
        row = summary[state["graph"], state["algorithm"]]
        require(int(row["completed_rounds"]) == len(samples), f"Resumo diverge dos dados: {name}")
        official = state["status"] == "complete" and len(samples) == 10 and state["requested_rounds"] == 10
        require(bool(row["official_mean_s"]) == official, f"Media oficial indevida ou ausente: {name}")
        if values:
            mean = statistics.mean(values)
            require(math.isclose(float(row["mean_s"]), mean, rel_tol=1e-12), f"Media incorreta: {name}")
            require(state["elapsed_wall_s"] >= sum(values), f"Tempo de parede menor que o medido: {name}")
            if official:
                require(math.isclose(float(row["official_mean_s"]), mean, rel_tol=1e-12), f"Media oficial incorreta: {name}")
        else:
            require(not row["mean_s"] and not row["official_mean_s"], f"Ausencia de medicao virou zero: {name}")
        if state["status"] != "complete":
            require(state["reason"] in ("memory_limit", "predicted_time_limit", "time_limit"), f"Limite sem justificativa: {name}")
    q1 = read_csv(directory / "q1_bellman_ford.csv")
    q2 = read_csv(directory / "q2_floyd_warshall.csv")
    expected_q1 = {(g, str(u), "100") for g in graphs for u in (10,20,30)}
    expected_q2 = {(g, str(u), str(v)) for g in graphs for u in (10,20,30,40,50) for v in (10,20,30,40,50)}
    for records, expected in ((q1, expected_q1), (q2, expected_q2)):
        require(len(records) == len(expected) and {(r["graph"],r["source"],r["target"]) for r in records} == expected,
                "Consultas ausentes ou duplicadas")
        for row in records:
            status = row["status"]
            require(status in ("finite", "unreachable", "negative_cycle", "not_computed"), "Estado de consulta invalido")
            if status == "finite":
                require(math.isfinite(float(row["distance"])), "Distancia finita invalida")
                path = [int(vertex) for vertex in row["path"].split("-")]
                require(path[0] == int(row["source"]) and path[-1] == int(row["target"]), "Extremos de caminho invalidos")
                require(all(1 <= vertex <= graphs[row["graph"]]["vertices"] for vertex in path), "Vertice de caminho invalido")
            elif status == "negative_cycle":
                require(row["distance"] == "-inf" and row["has_negative_cycle"] == "1" and not row["path"], "Ciclo negativo mal exportado")
            elif status == "unreachable":
                require(row["distance"] == "inf" and not row["path"], "Ausencia de caminho mal exportada")
            else:
                require(not row["distance"] and bool(row["reason"]), "Consulta nao calculada sem motivo")
    comparisons = read_csv(directory / "q4_comparacao.csv")
    require(len(comparisons) == len(graphs), "Tabela da questao 4 incompleta")
    for row in comparisons:
        both = bool(row["bf_mean_s"] and row["fw_mean_s"])
        require((row["conclusion"] != "comparacao_nao_concluida") == both, "Vencedor declarado sem comparacao completa")
        if both:
            require(math.isclose(float(row["bf_over_fw"]), float(row["bf_mean_s"]) / float(row["fw_mean_s"]), rel_tol=1e-12),
                    "Razao de tempos incorreta")
    names = ("q1_bellman_ford.csv", "q2_floyd_warshall.csv", "q3_tempos.csv", "q4_comparacao.csv", "environment.json", "states.json")
    certificate_path = directory / "w1_negative_cycle_certificate.json"
    if certificate_path.exists():
        certificate = json.loads(certificate_path.read_text(encoding="utf-8"))
        require(certificate.get("verified_with_decimal_input") and certificate["input_sha256"] == graphs["W1"]["sha256"],
                "Certificado negativo nao corresponde ao arquivo medido")
        require(all(row["status"] == "negative_cycle" for row in q1 if row["graph"] == "W1"),
                "Consultas de W1 divergem do certificado")
        names += ("w1_negative_cycle_certificate.json",)
    return {"status": "passed", "graphs": len(graphs), "configurations": len(states),
            "completed_rounds": sum(s["completed_rounds"] for s in states),
            "official_means": sum(s.get("official_mean_s") is not None for s in states),
            "artifact_sha256": {name: hashlib.sha256((directory / name).read_bytes()).hexdigest() for name in names}}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    directory = parser.parse_args().directory
    try:
        result = verify(directory)
    except (RuntimeError, ValueError, KeyError, OSError) as error:
        save_json(directory / "verification.json", {"status": "failed", "error": str(error)})
        raise
    save_json(directory / "verification.json", result)
    print(json.dumps(result, ensure_ascii=False))
