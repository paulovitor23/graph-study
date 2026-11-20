# Etapa 1 — Limpeza e otimização da graph-study

A etapa 1 foi concluída em 19/11/2026. O núcleo permanece em C++17, com as mesmas APIs de grafos não direcionados e os mesmos nomes e argumentos dos executáveis históricos. Grafos direcionados, Bellman–Ford e Floyd–Warshall pertencem à etapa 2 e ainda não foram implementados.

## Alterações aplicadas

- Executáveis organizados em `experiments/unweighted.cpp`, `weighted.cpp` e `researchers.cpp`; testes em `tests/`. O CMake referencia esses caminhos. Os relatórios históricos foram preservados.
- Utilitários efetivamente compartilhados em `experiments/common.hpp`: validação de inteiros, preparação das saídas, verificação de flush/fechamento e cronômetro. Os programas mantêm as perguntas específicas de cada trabalho.
- Parser em `graph_io.hpp`, usando `string_view` e `from_chars`, sem alocar um stream por linha. A leitura em lote, a redução de duplicatas pelo menor peso e a ordem original de vizinhos foram preservadas. O núcleo rejeita contagens negativas e estouros antes de alocar.
- Mediana dos graus por `nth_element` e seleção do maior elemento da metade inferior no caso par. Não é mais necessário ordenar todo o vetor de graus.
- Validação estrita dos argumentos e do CSV de origens: sinais negativos, estouros, linhas incompletas, origens repetidas e colunas extras são rejeitados. Semente zero continua válida. Nas CLIs, os inteiros devem conter somente dígitos; no arquivo de grafo, o sinal positivo explícito continua aceito.
- Cada executável registra carga e geração das saídas em arquivo adicional `_io.csv`, mantendo o formato dos arquivos anteriores. A geração das saídas inclui consultas auxiliares necessárias à exportação e não se confunde com os intervalos dos benchmarks.
- Avisos do compilador centralizados no CMake e adequados a GCC/Clang ou MSVC. As verificações dos testes permanecem ativas em Release.
- Ferramentas opcionais de inventário, medição com prazo e comparação de versões em `benchmarks/`. Python não é uma dependência do núcleo.

## Base preservada e método

A versão inicial era `fac2acbd97dc4d37fe6484d52ab5551c76d21830`, com árvore de trabalho limpa. Seus fontes e executáveis de referência foram preservados em `build/etapa1/baseline` e `baseline-build` antes das alterações. O manifesto e os registros de execução ficam em [TP3/etapa1](../../TP3/etapa1/).

Ambiente: AMD Ryzen 5 5625U, Ubuntu via WSL2 x86-64, kernel 6.6.87.2, glibc 2.39, GCC 13.3.0. O WSL informou 7.46 GiB de RAM total. Ambas as versões usaram `-std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic`. CMake 4.4.5 foi preparado em uma pasta de build, sem adicionar bibliotecas ao projeto.

As comparações finais alternam a ordem das versões em cada rodada, com processos separados e as mesmas três origens selecionadas pelo mesmo código e semente 242. Foram cinco rodadas por versão em W1/W3 e quinze na entrada sintética positiva de 2.000 vértices. Carga é medida uma vez por processo; buscas e Dijkstras são medidos para cada uma das três origens. Não houve concorrência entre os processos de benchmark.

O relógio é `steady_clock`. A chamada medida inclui inicialização dos resultados e alocações; hashing, conferência e exportação ficam fora do intervalo. A assinatura determinística considera contagens, vizinhos em ordem e pesos, estatísticas, pais, níveis, ordens de visita e distâncias. Todas as assinaturas coincidiram entre versões e rodadas. Isso complementa os testes com valores conhecidos; não é uma prova geral dos algoritmos.

Os valores abaixo são medianas de diagnóstico da manutenção, em milissegundos. Não são as médias de dez rodadas exigidas pelo estudo do TP3. Média, dispersão, mínimo e máximo também constam de [final_summary.csv](../../TP3/etapa1/final_summary.csv).

| Entrada / operação | Antes (ms) | Depois (ms) | Variação da mediana |
| --- | ---: | ---: | ---: |
| W1 / carga | 562,122 | 337,342 | −40,0% |
| W3 / carga | 7.151,951 | 4.975,688 | −30,4% |
| Sintético positivo / carga | 18,014 | 11,510 | −36,1% |
| W1 / estatísticas | 0,852 | 0,499 | −41,5% |
| W3 / estatísticas | 3,933 | 2,484 | −36,9% |
| W1 / BFS | 6,691 | 6,208 | −7,2% |
| W1 / DFS | 12,600 | 13,060 | +3,7% |
| W3 / BFS | 61,385 | 57,144 | −6,9% |
| W3 / DFS | 117,080 | 112,373 | −4,0% |
| W3 / heap | 133,773 | 124,506 | −6,9% |
| Sintético positivo / heap | 0,497 | 0,492 | −1,1% |
| Sintético positivo / vetor | 3,671 | 3,656 | −0,4% |

Os ganhos demonstrados concentram-se na carga e nas estatísticas. Os algoritmos de busca e Dijkstra não foram alterados; suas variações não justificam atribuir-lhes uma melhoria geral. A pequena piora da DFS em W1 não se repetiu em W3 nem no sintético. Não há controle de afinidade, frequência da CPU, temperatura ou cache do sistema. As rodadas preliminares tiveram tempos absolutos diferentes; por isso não foram misturadas à comparação final alternada.

## Memória e otimizações descartadas

A memória residente após a carga foi de aproximadamente 30,88 → 30,75 MiB em W1 e 295,75 → 295,75 MiB em W3. É memória do processo, incluindo runtime, e não memória exclusiva da representação. Não foi demonstrada uma redução relevante de RSS.

A compactação com `shrink_to_fit` foi testada isoladamente em W3, em cinco rodadas alternadas. Com compactação, a carga teve mediana de 4.980,844 ms; sem ela, 4.884,606 ms. O RSS permaneceu em 295,75 MiB nos dois casos. A compactação foi retirada: acrescentou cerca de 2% ao tempo de carga sem benefício de RSS observado. O ensaio está em [compaction_W3.csv](../../TP3/etapa1/compaction_W3.csv), onde `baseline` significa a variante com compactação e `candidate`, a variante sem compactação.

A reserva da pilha da DFS e a inicialização do heap foram avaliadas como candidatos, mas não modificadas. O custo e a memória das travessias não demonstraram um problema que justificasse mudar sua política nesta etapa. Foram preservadas as vistas de vizinhos, o heap indexado e a versão linear para comparação acadêmica. CSR, paralelismo e novos tipos numéricos não foram introduzidos.

O perfilador coleta RSS corrente e pico de RSS do processo no Linux; em outras plataformas os campos não suportados recebem zero. O pico via `getrusage` pode incluir o processo desde antes do `exec`, de modo que, nas entradas pequenas, o lançamento pelo Python pode dominar esse valor. O RSS após carga é a referência usada para comparar a compactação. Os JSONs registram o escopo e o ambiente de cada execução.

## Inventário dos cinco arquivos do TP3

| Grafo | Vértices | Linhas de arcos | Pares ordenados únicos | Linhas com peso negativo | Faixa dos pesos |
| --- | ---: | ---: | ---: | ---: | --- |
| W1 | 25.000 | 549.953 | 549.953 | 4.998 | −5 a 10 |
| W2 | 25.000 | 824.964 | 824.964 | 0 | 0 a 10 |
| W3 | 100.000 | 6.099.941 | 6.099.941 | 0 | 0 a 10 |
| W4 | 1.000.000 | 30.999.978 | 30.999.978 | 0 | 0 a 10 |
| W5 | 10.000.000 | 39.999.995 | 39.999.995 | 0 | 0 a 10 |

Não foram encontrados identificadores inválidos, linhas malformadas, laços ou pares ordenados repetidos. Arcos inversos são distintos nesse inventário; o resultado não equivale à contagem de arestas do carregador não direcionado atual. Os hashes SHA-256 e tamanhos estão em [inventory.csv](../../TP3/etapa1/inventory.csv). Cada processo de inventário teve prazo de 180 segundos; todos concluíram.

W1 não pode ser usado no benchmark de Dijkstra por conter pesos negativos. O perfil o exclui desse algoritmo, preservando a rejeição existente. Esse dado será importante na etapa 2, quando o grafo passar a ser carregado como direcionado.

## Validação

- CTest da versão final: quatro testes aprovados (`graph_tests`, falha proposital com `NDEBUG`, três CLIs e inventário).
- Testes de parsing cobrem CRLF, pesos opcionais, notação científica, sinal positivo no arquivo, negativos, vazio/zero, estouros, pesos não finitos e colunas extras. A mediana foi confrontada com ordenação completa em grafos de tamanhos pares e ímpares.
- As CLIs verificam criação de diretórios, árvores/componentes, caminhos, replay de origens, CSVs inválidos, pesquisadores e detecção de falha de escrita usando `/dev/full` no Linux.
- Comparação com os executáveis preservados: árvores, componentes, distâncias, caminhos, nomes e metadados funcionais coincidiram. Os campos de tempo, memória e caminhos de saída foram excluídos dessa comparação.
- Testes C++ executados com AddressSanitizer e UndefinedBehaviorSanitizer, sem diagnóstico de erro.
- `git diff --check` sem erros. A compilação nativa com MSVC e outras plataformas não foi executada.

Os logs estão em [final_checks.log](../../TP3/etapa1/final_checks.log). O inventário foi completo, mas os perfis dos algoritmos ficaram limitados a W1/W3 e uma entrada pequena positiva. Não foram executados benchmarks grandes com Dijkstra linear nem os algoritmos ainda não implementados do TP3.

## Reprodução

Na raiz da biblioteca, com CMake e Python 3.11+ disponíveis:

```sh
cmake -S . -B build/repro -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-DNDEBUG -DCMAKE_CXX_FLAGS_RELEASE=-O2 -DGRAPH_STUDY_BUILD_BENCHMARKS=ON
cmake --build build/repro -j2
ctest --test-dir build/repro --output-on-failure
python3 benchmarks/run_profile.py build/repro/graph_profile ../TP3/grafo_W_3.txt resultados/perfil_W3.csv --rounds 5 --sources 3 --timeout 120
python3 benchmarks/inventory.py build/repro/graph_inventory resultados/inventory.csv ../TP3/grafo_W_1.txt ../TP3/grafo_W_2.txt ../TP3/grafo_W_3.txt ../TP3/grafo_W_4.txt ../TP3/grafo_W_5.txt
```

Para comparar versões, compilar `benchmarks/profile.cpp` com o núcleo preservado e fornecer esse executável em `--baseline`. Nesta execução, foi usado `build/etapa1/baseline/graph_profile`. `compare_legacy.py` recebe os diretórios dos dois builds. `summarize_profile.py` recebe o CSV de saída e os arquivos brutos de entrada.

Os scripts de medições arquivados no TP2 são registros históricos que validam a versão antiga e não são o runner desta manutenção. Seus snapshots, hashes e resultados não foram reescritos. Para compilações atuais, usar o CMake ou os novos caminhos documentados no README.
