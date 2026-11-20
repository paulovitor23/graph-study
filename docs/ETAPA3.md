# Etapa 3: experimentos e respostas do TP3

Concluída em 20/11/2026, com os limites de memória e duração autorizados pelo usuário. Foram avaliadas 15 configurações nos cinco grafos: cinco concluíram dez rodadas, cinco foram puladas por memória e cinco foram interrompidas nos pilotos pela previsão de tempo. Há **50 rodadas completas e cinco médias oficiais**.

## Entregáveis

- [Respostas das quatro questões](../../TP3/etapa3/resultados/RESPOSTAS.md), geradas dos CSVs.
- [Relatório acadêmico de cinco páginas](../../TP3/output/pdf/relatorio_TP3.pdf), incluindo capa.
- [Roteiro de apresentação de oito minutos](../../TP3/etapa3/resultados/ROTEIRO_8_MIN.md).
- [Resultados brutos e estados](../../TP3/etapa3/resultados/), [manifesto](../../TP3/etapa3/manifest.json) e [conferência automática](../../TP3/etapa3/resultados/verification.json).
- [Pacote de entrega](../../TP3/entrega_TP3.zip), com fontes, evidências, relatório e instruções de reprodução; os arquivos gigantes dos grafos ficam fora do ZIP.

## Automação implementada

`experiments/directed.cpp` carrega o grafo direcionado em lista e executa três modos: Bellman–Ford até 100, Floyd–Warshall no grafo completo e Bellman–Ford repetido para todos os destinos. Uma rodada do último modo inclui as V chamadas; seu tempo não é dividido por V. Os resultados são consumidos por destino, sem outra matriz quadrática.

`scripts/run_tp3.py` supervisiona processos sequenciais, calcula hashes, confere tamanhos e contagens e preserva logs, pilotos, consultas e rodadas completas. A admissão usa estimativas conservadoras, 70% da memória disponível e duas horas por configuração, incluindo carga e pilotos. No Linux há também `RLIMIT_AS` e amostragem de RSS. Um limite interrompe somente a configuração corrente; as demais continuam. Uma pasta de saída preenchida nunca é sobrescrita.

`AlgorithmObserver` informa progresso somente nos pilotos. Nas dez rodadas oficiais não há observador. Os intervalos com `steady_clock` incluem a chamada do algoritmo, estruturas auxiliares e detecção de ciclos; excluem leitura, escrita, hashing, conferência e destruição dos resultados. As assinaturas devem coincidir entre rodadas, e os caminhos exportados têm seus arcos e custos conferidos.

`scripts/summarize_tp3.py` gera respostas e roteiro; `scripts/report_tp3_pdf.py` exporta o relatório com ReportLab; `scripts/verify_tp3.py` recalcula médias e valida contagens, estados, consultas e comparações. Somente dez rodadas completas geram média oficial. Consultas válidas dos pilotos são preservadas, mas seus tempos não entram na média.

## Resultados medidos

| Grafo | 10 → 100 | 20 → 100 | 30 → 100 | Média BF até 100 (s) | Desvio amostral (s) |
| --- | --- | --- | --- | --- | --- |
| W1 | −∞ | −∞ | −∞ | 132,408499 | 10,920522 |
| W2 | 10,47 | 8,48 | 8,71 | 0,075824 | 0,021102 |
| W3 | 4,63 | 5,42 | 4,67 | 0,739411 | 0,121666 |
| W4 | 9,04 | 8,83 | 9,23 | 7,054624 | 0,411194 |
| W5 | 37,85 | 42,63 | 37,86 | 24,375088 | 1,719951 |

Cada média usa dez rodadas. Em W1 não existe mínimo finito nessas consultas: o ciclo **10 → 11278 → 10**, de custo decimal exato **−2,00**, é alcançável das três origens e permite alcançar 100. O [certificado](../../TP3/etapa3/resultados/w1_negative_cycle_certificate.json) foi conferido independentemente contra os pesos originais usando `Decimal`, incluindo as conexões. `validation/cycle_certificate.cpp` apenas procura ciclos curtos para produzir esse certificado; a ausência de um certificado nesse auxiliar não prova ausência de ciclos. A detecção geral pertence à biblioteca.

Na primeira rodada, W1 realizou 24.999 passos e examinou 13.549.263.978 arcos na busca até 100; W4 realizou 36 passos e examinou 148.909.037 arcos. O ciclo negativo impede a parada antecipada em W1, explicando por que esse grafo menor demorou mais.

## Casos inviáveis e respostas das questões 2 e 4

Todos os grafos foram representados por lista de adjacência. Isso permite executar Bellman–Ford nos maiores casos, mas não elimina os buffers quadráticos de Floyd–Warshall.

| Grafo | Buffers de FW, sem o grafo (GiB) | Previsão de dez rodadas BF todos os pares (h) |
| --- | --- | --- |
| W1 | 9,31 | 12.787,58 |
| W2 | 9,31 | 4,05 |
| W3 | 149,01 | 193,84 |
| W4 | 14.901,17 | 18.582,97 |
| W5 | 1.490.116,19 | 646.667,67 |

FW foi pulado antes da alocação (`skipped / memory_limit`): o orçamento efetivo ficou próximo de 4,86 GiB, e o WSL dispunha de 7,46 GiB totais. Seus 125 pares solicitados constam como não calculados, com motivo explícito. Não se substituiu o grafo por um subgrafo de cinco vértices.

BF para todos os pares foi interrompido nos pilotos (`aborted / predicted_time_limit`), sem nenhuma rodada oficial. A projeção usa o menor tempo completo entre até quatro destinos amostrados, multiplicado por V e dez rodadas. Em W1 o piloto foi interrompido pela extrapolação dos passos, assumindo persistência das atualizações; essa hipótese é registrada. As projeções são estimativas, não tempos medidos. **Não há vencedor empírico na questão 4**, pois nenhuma comparação completa foi possível. A análise assintótica e a justificativa dos limites estão nas respostas e no relatório.

## Ambiente, testes e rastreabilidade

A execução ocorreu entre 00:45:47 e 01:24:26 UTC de 20/11/2026 (21:45 a 22:24 de 19/11 em São Paulo). Ambiente: Ryzen 5 5625U, WSL2 Linux x86-64, GCC 13.3.0, C++17, `-O2 -DNDEBUG -Wall -Wextra -Wpedantic`. A mesma entrada permanece em memória entre rodadas; cada chamada cria novos resultados. Não houve controle de afinidade, frequência, temperatura ou cache. Os tempos descrevem esta máquina e execução.

Os seis testes CTest passaram em Release, incluindo as CLIs históricas, verificações ativas com `NDEBUG`, os algoritmos direcionados e o executor. A automação exercita dez rodadas, médias, ciclos negativos, os três modos, continuação após limites, preservação de rodadas antes de uma interrupção, memória, prazo e recusa de sobrescrita. A projeção de todos os destinos tem regressão para excluir o custo fixo de preparação da taxa por destino. Os testes direcionados também passaram com AddressSanitizer e UndefinedBehaviorSanitizer, conferindo 38.340 pares contra referências independentes. Evidência: [final_checks.log](../../TP3/etapa3/final_checks.log). Não foi validada uma compilação MSVC.

`measurement_sources/` e `measurement_manifest.json` preservam as fontes exatas usadas ao iniciar a execução; `final_sources/` preserva a versão final com documentação e verificadores. Depois do início foi refinada uma projeção do supervisor para descontar o preparo fixo da taxa por destino. Essa ramificação não foi usada nos grafos medidos: os registros mostram somente `fastest_completed_pilot` ou `pass_extrapolation_assuming_no_early_stop`. O núcleo e o executor cronometrado permanecem idênticos ao snapshot de medição. O pacote registra hashes das duas versões e do executável medido.

Para reproduzir, use os comandos do [README](../README.md#estudos-automatizados-do-tp3) e uma pasta de saída nova. O manifesto identifica os cinco arquivos por SHA-256. O ZIP inclui um `LEIA-ME.md` com os caminhos e comandos; as fontes modificadas continuam locais, sem publicação no repositório remoto.
