"""Export a five-page academic report from the TP3 CSV/JSON evidence (requires reportlab)."""

import argparse
import csv
import json
from pathlib import Path
from xml.sax.saxutils import escape

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, Image


def read(path):
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source))


def fmt(value, decimals=3):
    if value in (None, ""):
        return "N/D"
    if str(value) == "-inf":
        return "-infinito"
    if str(value) == "inf":
        return "Sem caminho"
    return f"{float(value):,.{decimals}f}".replace(",", "X").replace(".", ",").replace("X", ".")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--logo", type=Path)
    args = parser.parse_args()
    env = json.loads((args.directory / "environment.json").read_text(encoding="utf-8"))
    if not env.get("finished_utc"):
        raise RuntimeError("A execucao deve terminar antes da exportacao do relatorio")
    q1, q3, q4 = [read(args.directory / name) for name in
                  ("q1_bellman_ford.csv", "q3_tempos.csv", "q4_comparacao.csv")]
    graphs = env["manifest"]["graphs"]
    states = {(row["graph"], row["algorithm"]): row for row in q3}
    queries = {(row["graph"], row["source"]): row for row in q1}
    font_dir = Path("C:/Windows/Fonts")
    if not font_dir.exists():
        font_dir = Path("/mnt/c/Windows/Fonts")
    if (font_dir / "times.ttf").exists():
        for name, filename in [("ReportSerif", "times.ttf"), ("ReportSerif-Bold", "timesbd.ttf"),
                               ("ReportSerif-Italic", "timesi.ttf")]:
            pdfmetrics.registerFont(TTFont(name, str(font_dir / filename)))
        pdfmetrics.registerFontFamily("ReportSerif", normal="ReportSerif", bold="ReportSerif-Bold",
                                      italic="ReportSerif-Italic", boldItalic="ReportSerif-Bold")
        regular, bold = "ReportSerif", "ReportSerif-Bold"
    else:
        regular, bold = "Times-Roman", "Times-Bold"
    body = ParagraphStyle("body", fontName=regular, fontSize=11.5, leading=14.5, alignment=TA_JUSTIFY, spaceAfter=9)
    heading = ParagraphStyle("heading", fontName=bold, fontSize=15, leading=18, spaceAfter=14)
    small = ParagraphStyle("small", parent=body, fontSize=10, leading=12, spaceAfter=7)
    cell = ParagraphStyle("cell", parent=small, alignment=TA_CENTER, spaceAfter=0)
    title = ParagraphStyle("title", fontName=bold, fontSize=24, leading=29, alignment=TA_CENTER, spaceAfter=14)
    centered = ParagraphStyle("centered", parent=body, alignment=TA_CENTER)
    story = []
    def p(text, style=body):
        story.append(Paragraph(text, style))
    def h(text):
        p(text, heading)
    def table(headers, data, widths=None):
        values = [[Paragraph(escape(str(value)), cell) for value in row] for row in [headers, *data]]
        tab = Table(values, colWidths=widths, repeatRows=1, hAlign="CENTER")
        tab.setStyle(TableStyle([
            ("LINEABOVE", (0,0), (-1,0), .8, colors.black),
            ("LINEBELOW", (0,0), (-1,0), .5, colors.black),
            ("LINEBELOW", (0,-1), (-1,-1), .8, colors.black),
            ("BACKGROUND", (0,0), (-1,0), colors.HexColor("#eeeeee")),
            ("TOPPADDING", (0,0), (-1,-1), 7), ("BOTTOMPADDING", (0,0), (-1,-1), 7),
            ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ]))
        story.extend([tab, Spacer(1, 12)])

    if args.logo and args.logo.exists():
        logo = Image(str(args.logo))
        logo.drawHeight = logo.imageHeight * (14 * cm / logo.imageWidth)
        logo.drawWidth = 14 * cm
        logo.hAlign = "CENTER"
        story.append(logo)
    else:
        p("Universidade Federal do Rio de Janeiro", centered)
    story.append(Spacer(1, 35))
    p("COS242 - Teoria dos Grafos", centered)
    story.append(Spacer(1, 15))
    p("Relatório do Trabalho Prático 3", title)
    p("Grafos direcionados e caminhos mínimos", centered)
    story.append(Spacer(1, 35))
    p("<b>Alunos</b><br/>Paulo Vitor Couto Doederlein<br/>Erik Branco Queiroz")
    p("<b>Professor</b><br/>Daniel Ratton Figueiredo")
    story.append(Spacer(1, 30))
    p("Rio de Janeiro, novembro de 2026", centered)
    story.append(PageBreak())

    h("1. Implementação e método")
    p("A biblioteca C++17 foi estendida para grafos direcionados ponderados em lista de adjacência. "
      "A ordem dos identificadores em cada linha define o arco. Arcos inversos são independentes; duplicatas mantêm o menor peso. "
      "BFS, DFS e os dois Dijkstras usam as arestas de saída, preservando as chamadas não direcionadas dos trabalhos anteriores.")
    p("Bellman-Ford calcula distâncias <b>até um destino fixo</b>. Um vetor substitui a matriz por número de arestas; "
      "as fronteiras dos vértices alterados limitam os relaxamentos aos predecessores afetados e permitem parada antecipada. "
      "Floyd-Warshall atualiza uma matriz de distâncias e mantém predecessores por origem. Os dois distinguem distância finita, "
      "ausência de caminho e influência de ciclo negativo, além de informar a existência global de ciclos negativos.")
    table(["Grafo", "Vértices", "Arcos"], [[g["id"], f"{g['vertices']:,}".replace(",","."),
          f"{g['edges']:,}".replace(",",".")] for g in graphs], [50,155,155])
    p("Ambiente: AMD Ryzen 5 5625U, Ubuntu via WSL2 x86-64, GCC 13.3.0; compilação com C++17, -O2 e -DNDEBUG. "
      "Cada configuração viável tem dez rodadas. O relógio steady_clock mede a chamada, incluindo preparo dos vetores, "
      "predecessores e detecção de ciclos. Leitura, escrita, hashing, validação e destruição dos resultados ficam fora do intervalo. "
      "Pilotos e previsões são separados das medições oficiais.")
    p("O limite operacional é de duas horas por configuração, incluindo preparação e pilotos. A admissão usa estimativa de memória "
      "e 70% da memória disponível; no Linux também há limite de espaço de endereçamento e amostragem de RSS. "
      "Execuções inviáveis são puladas ou interrompidas com justificativa e preservação dos resultados concluídos.")
    p("Repositório-base: <link href='https://github.com/paulovitor23/graph-study' color='blue'>"
      "https://github.com/paulovitor23/graph-study</link>. A versão local exata, os hashes e os testes acompanham o snapshot desta execução; "
      "as alterações locais não foram publicadas por esta tarefa.", small)
    story.append(PageBreak())

    h("2. Distâncias solicitadas")
    p("<b>Questão 1.</b> Bellman-Ford de todos os vértices até 100, com consultas nas origens 10, 20 e 30:")
    table(["Grafo", "10 para 100", "20 para 100", "30 para 100"],
          [[g["id"], *[fmt(queries[g["id"],str(u)]["distance"],2) for u in (10,20,30)]] for g in graphs],
          [45,135,135,135])
    p("O valor -infinito indica que um ciclo negativo pode ser atravessado antes de alcançar o destino; "
      "nesse caso não existe caminho mínimo de custo finito. N/D identifica consulta não calculada. "
      "Os caminhos finitos completos e os estados estão em q1_bellman_ford.csv. Os pilotos válidos podem fornecer respostas, "
      "mas seus tempos não entram na média de dez rodadas.")
    certificate_path = args.directory / "w1_negative_cycle_certificate.json"
    if certificate_path.exists():
        certificate = json.loads(certificate_path.read_text(encoding="utf-8"))
        if certificate.get("verified_with_decimal_input"):
            cycle = " - ".join(map(str, certificate["cycle"]))
            p(f"Verificação adicional de W1: ciclo {cycle}, custo decimal exato {certificate['cycle_cost_decimal'].replace('.', ',')}. "
              "Foram conferidas conexões das três origens até o ciclo e dele até 100 diretamente nos pesos do arquivo.", small)
    p("<b>Questão 2.</b> Floyd-Warshall deve considerar o grafo completo para responder aos pares ordenados de {10,20,30,40,50}. "
      "A tabela abaixo apresenta a viabilidade da execução, não distâncias substitutas.")
    table(["Grafo", "Matrizes e índices (GiB)", "Orçamento total (GiB)", "Estado"],
          [[g["id"], fmt(float(states[g["id"],"floyd"]["matrix_bytes"])/1024**3,2),
            fmt(float(states[g["id"],"floyd"]["memory_budget_bytes"])/1024**3,2),
            "Memória insuficiente" if states[g["id"],"floyd"]["reason"] == "memory_limit" else states[g["id"],"floyd"]["status"]]
           for g in graphs], [45,130,125,150])
    p("Usar lista de adjacência não elimina a matriz interna de distâncias. Quando a estimativa excede o orçamento, "
      "o algoritmo não é iniciado. Os 25 pares de cada grafo ficam explicitamente marcados em q2_floyd_warshall.csv. "
      "Não foi aplicado Floyd-Warshall somente aos cinco vértices, pois isso poderia excluir caminhos que passam por outros vértices.")
    story.append(PageBreak())

    h("3. Tempos médios de execução")
    p("<b>Questão 3.</b> Valores em segundos. A média oficial é apresentada apenas com dez rodadas completas; "
      "campos N/D não representam tempo zero. O desvio é amostral.")
    table(["Grafo", "BF até 100: média", "Desvio BF", "Rodadas BF", "FW: média"],
          [[g["id"], fmt(states[g["id"],"bf_target"]["official_mean_s"]),
            fmt(states[g["id"],"bf_target"]["stdev_s"]), states[g["id"],"bf_target"]["completed_rounds"] + "/10",
            fmt(states[g["id"],"floyd"]["official_mean_s"])] for g in graphs], [40,120,100,80,110])
    p("Bellman-Ford executa chamadas completas com destino 100, sempre com novos resultados. A direção das consultas "
      "é origem para destino: executar um algoritmo a partir de 100 produziria outro problema. A detecção global complementar "
      "de ciclos integra o tempo quando necessária.")
    p("A presença de ciclos negativos pode impedir a parada antecipada e exigir os V-1 passos. Por isso o número de vértices "
      "ou de arcos, isoladamente, não determina o tempo observado. relaxation_passes e examined_edges registram o trabalho "
      "da busca até o destino, sem contar o preparo e a verificação global complementar.")
    example_counters = []
    for graph_id in ("W1", "W4"):
        path = args.directory / graph_id / "bf_target" / "timings.csv"
        if path.exists() and read(path):
            sample = read(path)[0]
            example_counters.append(f"{graph_id}: {int(sample['relaxation_passes']):,} passos e "
                                    f"{int(sample['examined_edges']):,} arcos examinados".replace(",", "."))
    if example_counters:
        p("Primeira rodada: " + "; ".join(example_counters) + ".", small)
    p("Os arquivos timings.csv contêm as durações individuais; pilots.csv registra os destinos e tempos dos pilotos. "
      "As previsões e os limites estão em q3_tempos.csv e nos arquivos state.json. As assinaturas das distâncias são conferidas "
      "entre rodadas, e os caminhos exportados têm suas arestas e custos validados fora do intervalo cronometrado.")
    p("As execuções são sequenciais, com a mesma entrada em memória entre rodadas. Não houve controle de afinidade, frequência "
      "da CPU, temperatura ou cache do sistema. As medições descrevem esta máquina e esta execução; não são garantias universais de desempenho.")
    story.append(PageBreak())

    h("4. Comparação de todos os pares")
    p("<b>Questão 4.</b> Uma rodada da abordagem Bellman-Ford repete o algoritmo para todos os destinos. As colunas são consumidas "
      "individualmente, sem materializar outra matriz quadrática. O total da rodada não é dividido por V.")
    table(["Grafo", "BF: rodadas completas", "Previsão de 10 rodadas BF (h)", "Comparação medida"],
          [[g["id"], states[g["id"],"bf_all_pairs"]["completed_rounds"] + "/10",
            fmt(float(states[g["id"],"bf_all_pairs"]["estimated_total_s"])/3600,2)
                if states[g["id"],"bf_all_pairs"]["estimated_total_s"] else "N/D",
            "Não concluída" if next(r for r in q4 if r["graph"] == g["id"])["conclusion"] == "comparacao_nao_concluida" else
            next(r for r in q4 if r["graph"] == g["id"])["conclusion"]] for g in graphs], [45,105,140,160])
    p("As projeções de todos os pares usam o menor tempo de uma amostra de destinos multiplicado por V e pelo número de rodadas. "
      "Quando um piloto não conclui, a previsão pode extrapolar os passos observados, assumindo a persistência das atualizações; "
      "essa hipótese e o método são registrados. Previsões são estimativas sujeitas a variação e não substituem médias medidas.")
    p("<b>Não há vencedor empírico nos casos cuja comparação não foi concluída.</b> Floyd-Warshall requer tempo Θ(V³) e memória "
      "Θ(V²); repetir Bellman-Ford para todos os destinos tem pior caso O(V²E). As fronteiras podem reduzir o trabalho prático "
      "de BF, mas a repetição do preparo e das buscas em grafos grandes ainda pode demandar muitas horas. A complexidade explica "
      "o risco de custo; ela não permite inventar o tempo de uma execução não realizada.")
    p("Validação: referências independentes na etapa 2 conferiram 38.340 pares; os testes da etapa 3 exercitam dez rodadas, "
      "médias, ciclos negativos, comparação dos pares e continuação após limites. Ambiente, fontes, hashes, logs e comandos "
      "acompanham os resultados para reprodução.")
    p("Referências da implementação: aulas 13 (Floyd-Warshall, páginas 17-19) e 14 (Bellman-Ford, páginas 11-12) "
      "e enunciado do Trabalho de Curso - Parte 3. Anexos de dados: q1_bellman_ford.csv, q2_floyd_warshall.csv, "
      "q3_tempos.csv e q4_comparacao.csv.", small)

    def footer(canvas, doc):
        if doc.page > 1:
            canvas.setFont(regular, 9)
            canvas.drawCentredString(A4[0]/2, 1.35*cm, str(doc.page))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    document = SimpleDocTemplate(str(args.output), pagesize=A4, rightMargin=2.5*cm, leftMargin=2.5*cm,
                                 topMargin=2.4*cm, bottomMargin=2.2*cm,
                                 title="Relatório do Trabalho Prático 3 - Teoria dos Grafos",
                                 author="Paulo Vitor Couto Doederlein e Erik Branco Queiroz")
    document.build(story, onFirstPage=footer, onLaterPages=footer)


if __name__ == "__main__":
    main()
