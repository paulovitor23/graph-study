"""Generate the study answers directly from a completed run directory."""

import argparse
import csv
import json
from pathlib import Path


def rows(path):
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source))


def number(value, digits=6):
    if value in (None, ""):
        return "N/D"
    if str(value) == "-inf":
        return "sem mínimo finito (ciclo negativo)"
    if str(value) == "inf":
        return "sem caminho"
    return f"{float(value):.{digits}f}".replace(".", ",")


def table(headers, data):
    return "\n".join(["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |",
                       *["| " + " | ".join(map(str, row)) + " |" for row in data]])


def generate(directory):
    q1 = rows(directory / "q1_bellman_ford.csv")
    q2 = rows(directory / "q2_floyd_warshall.csv")
    q3 = rows(directory / "q3_tempos.csv")
    q4 = rows(directory / "q4_comparacao.csv")
    environment = json.loads((directory / "environment.json").read_text(encoding="utf-8"))
    graphs = environment["manifest"]["graphs"]
    ids = [graph["id"] for graph in graphs]
    state = {(row["graph"], row["algorithm"]): row for row in q3}
    queries = {(row["graph"], row["source"], row["target"]): row for row in q1}
    answers = ["# Respostas do TP3", "",
        "Resultados gerados automaticamente dos CSVs desta execução. Os grafos são direcionados e foram carregados por lista de adjacência. "
        "Uma média oficial exige dez rodadas completas. N/D indica teste não concluído ou não executado; não é tempo zero.", "",
        "## Questão 1 — Bellman–Ford até o vértice 100", "",
        table(["Grafo", "10 → 100", "20 → 100", "30 → 100"],
              [[graph, *[number(queries[graph, str(u), "100"]["distance"], 2) for u in (10,20,30)]] for graph in ids]), "",
        "Distâncias finitas, estados e caminhos completos estão em [q1_bellman_ford.csv](q1_bellman_ford.csv). "
        "O ciclo negativo afeta somente as consultas que podem atravessá-lo e alcançar o destino. Um resultado válido obtido no piloto "
        "pode responder à consulta, mas seu tempo não integra a média oficial.", "",
        "## Questão 2 — Floyd–Warshall entre os pares de {10,20,30,40,50}", ""]
    certificate_path = directory / "w1_negative_cycle_certificate.json"
    if certificate_path.exists():
        certificate = json.loads(certificate_path.read_text(encoding="utf-8"))
        if certificate.get("verified_with_decimal_input"):
            cycle = " → ".join(map(str, certificate["cycle"]))
            insertion = answers.index("## Questão 2 — Floyd–Warshall entre os pares de {10,20,30,40,50}")
            answers[insertion:insertion] = [f"Verificação complementar de W1: ciclo **{cycle}**, com custo decimal exato "
                f"**{certificate['cycle_cost_decimal']}**. Foram conferidos caminhos das três origens até o ciclo e dele até 100 "
                "diretamente no arquivo, usando aritmética decimal. "
                "[Certificado e caminhos](w1_negative_cycle_certificate.json).", ""]
    for graph in ids:
        record = state[graph, "floyd"]
        available = [row for row in q2 if row["graph"] == graph and row["status"] != "not_computed"]
        if available:
            lookup = {(row["source"], row["target"]): row for row in available}
            answers += [f"### {graph}", "", table(["Origem / destino", "10", "20", "30", "40", "50"],
                [[u, *[number(lookup[str(u),str(v)]["distance"], 2) for v in (10,20,30,40,50)]] for u in (10,20,30,40,50)]), ""]
        else:
            answers += [f"- **{graph}:** {record['status']} / {record['reason']}. "
                        f"Estimativa total: {int(record['memory_estimate_bytes']) / 1024**3:.2f} GiB; "
                        f"orçamento: {int(record['memory_budget_bytes']) / 1024**3:.2f} GiB."]
    answers += ["", "A matriz interna do algoritmo é necessária mesmo usando lista para o grafo. "
                "Os 25 pares ordenados de cada grafo, inclusive a diagonal, constam de [q2_floyd_warshall.csv](q2_floyd_warshall.csv). "
                "Quando o teste é inviável, todos ficam marcados como não calculados. Não foi usado um subgrafo reduzido para substituir o algoritmo completo.", "",
        "## Questão 3 — Média de dez rodadas, em segundos", "",
        table(["Grafo", "BF até 100: média (s)", "Rodadas BF", "Desvio BF (s)", "FW: média (s)", "Estado FW"],
              [[graph, number(state[graph,"bf_target"]["official_mean_s"]),
                f"{state[graph,'bf_target']['completed_rounds']}/10", number(state[graph,"bf_target"].get("stdev_s")),
                number(state[graph,"floyd"]["official_mean_s"]),
                state[graph,"floyd"]["status"] + " / " + state[graph,"floyd"]["reason"]] for graph in ids]), "",
        "[q3_tempos.csv](q3_tempos.csv) preserva médias, medianas, dispersão, previsões e limites; cada pasta de algoritmo contém "
        "os tempos individuais em timings.csv, os pilotos separados, as consultas e os eventos. A leitura dos arquivos, a escrita, "
        "a validação dos caminhos, o hashing e a destruição dos resultados ficam fora dos tempos dos algoritmos. "
        "O preparo das estruturas auxiliares e a detecção de ciclos negativos fazem parte da chamada medida.", "",
        "## Questão 4 — Bellman–Ford para todos os pares versus Floyd–Warshall", "",
        table(["Grafo", "Estado BF todos os pares", "Previsão de 10 rodadas BF (h)", "Média BF (s)", "Média FW (s)", "Conclusão"],
              [[record["graph"], record["bf_status"] + " / " + record["bf_reason"],
                number(float(state[record["graph"],"bf_all_pairs"]["estimated_total_s"]) / 3600, 2)
                    if state[record["graph"],"bf_all_pairs"]["estimated_total_s"] else "N/D",
                number(record["bf_mean_s"]), number(record["fw_mean_s"]), record["conclusion"]] for record in q4]), ""]
    counters = []
    for graph in ids:
        samples = rows(directory / graph / "bf_target" / "timings.csv") if (directory / graph / "bf_target" / "timings.csv").exists() else []
        if samples:
            counters.append([graph, samples[0]["relaxation_passes"], samples[0]["examined_edges"]])
    position = answers.index("## Questão 4 — Bellman–Ford para todos os pares versus Floyd–Warshall")
    answers[position:position] = ["Trabalho da busca até 100 na primeira rodada (não inclui preparo nem detecção global complementar):", "",
        table(["Grafo", "Passos de relaxamento", "Arcos examinados"], counters), "",
        "Ciclos negativos podem impedir a parada antecipada, enquanto grafos sem essa influência podem convergir em poucos passos. "
        "Por isso um grafo menor pode demandar mais tempo que um grafo maior.", ""]
    if all(record["conclusion"] == "comparacao_nao_concluida" for record in q4):
        answers += ["Não foi possível apontar um vencedor empírico nos grafos do estudo: faltam execuções completas das duas abordagens "
                    "para o mesmo problema. Os casos inviáveis foram informados e pulados conforme a política definida. As previsões não são médias medidas.", ""]
    answers += ["Uma rodada de Bellman–Ford para todos os pares executa o algoritmo para **todos os destinos**, consumindo cada coluna sem "
                "materializar outra matriz quadrática. Seu pior caso é O(V²E), enquanto Floyd–Warshall custa Θ(V³) em tempo e Θ(V²) em memória. "
                "Atualizações seletivas podem reduzir o trabalho prático de BF, mas a repetição do preparo e das buscas para V destinos torna "
                "a abordagem onerosa nesses tamanhos. A ordem assintótica não substitui uma comparação cronometrada que não pôde ser concluída.", "",
        "## Método e rastreabilidade", "",
        f"- Início UTC: {environment['started_utc']}; término UTC: {environment.get('finished_utc', 'em execução')}.",
        "- Relógio steady_clock; unidade segundo; mesma entrada em memória entre rodadas, com novo resultado a cada execução.",
        f"- Limite de {environment['manifest']['time_limit_s']} segundos por configuração, incluindo preparação e pilotos. Processos de algoritmos são executados sequencialmente.",
        "- Memória: estimativa conservadora antes da execução; orçamento efetivo registrado por configuração. No Linux há limite de espaço de endereçamento e amostragem de RSS.",
        "- Projeção BF todos os pares: menor tempo de uma amostra de destinos × V × dez rodadas. Pilotos interrompidos podem usar extrapolação dos passos observados; o método e sua hipótese ficam no estado da configuração.",
        "- Os observadores de progresso são usados somente nos pilotos. Nas rodadas oficiais não há callback de I/O dentro do algoritmo.",
        "- Sem controle de frequência, temperatura, afinidade de CPU ou cache do sistema; as medições descrevem esta execução local.",
        "- Ambiente, hashes dos arquivos e executável: [environment.json](environment.json). Estados: [states.json](states.json).",
        "- Implementação-base: https://github.com/paulovitor23/graph-study. A versão local exata e os testes acompanham o snapshot da etapa 3; esta execução não publica alterações no repositório remoto.", ""]
    (directory / "RESPOSTAS.md").write_text("\n".join(answers), encoding="utf-8")
    speech = ["# Roteiro de apresentação do TP3 em oito minutos", "",
        "## 0:00–0:40 — Problema e objetivo", "",
        "Apresentar a extensão da biblioteca para grafos direcionados com pesos. Explicar que o estudo exige caminhos até um destino e entre todos os pares.", "",
        "## 0:40–2:00 — Representação e integração", "",
        "Mostrar a lista de adjacência e a direção primeiro identificador → segundo identificador. Explicar por que BFS, DFS e Dijkstra reutilizam o acesso aos vizinhos. Mencionar os testes de assimetria e a compatibilidade com os TPs anteriores.", "",
        "## 2:00–3:15 — Bellman–Ford", "",
        "Mostrar o destino 100 e a propagação para os predecessores. Explicar vetor único, fronteiras dos vértices alterados e parada antecipada. Distinguir ciclo negativo em outra componente de uma consulta sem mínimo finito.", "",
        "## 3:15–4:15 — Floyd–Warshall e memória", "",
        "Mostrar matriz única e predecessores. Usar a tabela de viabilidade para explicar por que a lista do grafo não elimina a matriz de distâncias. Indicar quais testes foram pulados.", "",
        "## 4:15–5:45 — Distâncias e tempos", "",
        "Apresentar a tabela da questão 1 e as médias oficiais da questão 3. Explicar que cada média tem dez rodadas e exclui leitura/escrita. Identificar estados de ciclo negativo, ausência de caminho e resultado não calculado.", "",
        "## 5:45–7:10 — Todos os pares e limites", "",
        "Explicar que BF deve ser repetido para todos os destinos. Mostrar previsões separadas de medições e os motivos de interrupção. Evitar declarar vencedor quando a comparação completa não foi obtida.", "",
        "## 7:10–8:00 — Validação e reprodução", "",
        "Resumir referências independentes e os testes automatizados da etapa 3. Mostrar CSVs, logs, hashes, snapshot e comandos de reprodução. Encerrar com o resultado alcançado e as limitações documentadas.", ""]
    (directory / "ROTEIRO_8_MIN.md").write_text("\n".join(speech), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    generate(parser.parse_args().directory)
