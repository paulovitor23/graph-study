# graph-study

Biblioteca em C++17 para manipulação de grafos não direcionados, desenvolvida
para o trabalho de Teoria dos Grafos.

## Funcionalidades implementadas

- representação por lista de adjacência;
- representação por matriz de adjacência;
- arestas com pesos reais;
- leitura de arquivos com ou sem a coluna de peso;
- número de vértices e arestas;
- grau de cada vértice;
- grau mínimo, máximo, médio e mediana dos graus.
- busca em largura (BFS), com pai, nível e ordem de visita;
- busca em profundidade (DFS), com pai, nível e ordem de visita;
- componentes conexas, ordenadas por tamanho decrescente;
- distância entre dois vértices usando BFS;
- Dijkstra com seleção linear ou heap mínimo indexado;
- distâncias e reconstrução de caminhos mínimos;
- diâmetro exato;
- aproximação do diâmetro para grafos grandes.

## Formato de entrada

A primeira linha contém o número de vértices. Cada linha seguinte contém os
dois vértices e, opcionalmente, o peso da aresta:

```text
5
1 2 0.1
2 5 0.2
5 3 5
```

Quando o peso é omitido, a aresta recebe peso `1.0`, mantendo compatibilidade
com os arquivos da primeira parte. Pesos negativos podem ser armazenados; a
execução do Dijkstra os rejeita com uma mensagem clara. Quando o arquivo
repete o mesmo par de vértices, a biblioteca mantém o menor peso.

## Organização

- `graph.hpp/.cpp`, `algorithms.hpp/.cpp`: núcleo da biblioteca;
- `graph_io.hpp`: parser interno do formato de entrada;
- `experiments/`: programas do TP1/TP2 e utilitários comuns;
- `tests/`: regressões funcionais e das interfaces de linha de comando;
- `benchmarks/`: perfil de desempenho e inventário dos arquivos;
- `docs/`: decisões e resultados da manutenção;
- `build/` e `resultados/`: arquivos gerados, fora do versionamento.

Os nomes dos executáveis e seus argumentos continuam os mesmos. Compilações
manuais devem usar os novos caminhos dos fontes em `experiments/` e `tests/`.

## Compilação e testes

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Python 3 é opcional para compilar a biblioteca e executar seus testes C++.
Quando encontrado pelo CMake, habilita também o teste das três CLIs. Os scripts
de medição usam Python 3.11 ou posterior. GCC/Clang recebem `-Wall -Wextra
-Wpedantic`; MSVC recebe `/W4 /permissive- /utf-8`. O parser requer suporte a
`std::from_chars` para números reais na biblioteca padrão C++17.

Contagens e identificadores negativos, estouros, pesos não finitos e colunas
extras são rejeitados antes de usar os valores. Números reais usam ponto decimal;
pesos em notação científica e arquivos CRLF são aceitos. Nas CLIs, contagens,
identificadores e sementes são formados somente por dígitos; semente zero é válida.

## Experimentos da Parte 1

Cada execução analisa um grafo usando uma representação. As representações
devem ser executadas em processos separados para que as medições de memória
não interfiram entre si.

```bash
./build/graph_experiments \
    ../grafo_1.txt \
    list \
    resultados/grafo_1_lista \
    100 \
    exact \
    1
```

Argumentos:

1. caminho do arquivo do grafo;
2. representação: `list` ou `matrix`;
3. prefixo dos arquivos de saída;
4. quantidade de BFS e DFS cronometradas, normalmente `100`;
5. diâmetro: `exact`, `approx` ou `skip`;
6. raiz das árvores BFS e DFS, numerada a partir de 1 (padrão: `1`).

O programa produz:

- `<prefixo>_summary.txt`, com estatísticas, memória, tempos, consultas e
  diâmetro;
- `<prefixo>_components.txt`, com tamanho e vértices de cada componente;
- `<prefixo>_search_trees.txt`, com pai e nível de cada vértice nas árvores
  BFS e DFS da raiz escolhida; `-` indica vértice não alcançado.

Leitura e escrita não fazem parte das medições dos algoritmos. Matrizes que
excedam 60% da memória física são rejeitadas antes da alocação.
O campo de memória registra o conjunto residente do processo em MiB (identificado
como `mb` no arquivo de saída), inclusive no Windows.

Os três programas também escrevem `<prefixo>_io.csv` (no estudo de pesquisadores,
`<saida.csv>_io.csv`). `load_ms` mede a carga do grafo; no estudo de pesquisadores
inclui também a leitura dos nomes. `export_ms` mede a geração e escrita das saídas,
incluindo consultas auxiliares usadas para produzi-las, e exclui a escrita do
próprio arquivo de tempos. Esses intervalos ficam separados dos benchmarks dos
algoritmos. Falhas no flush/fechamento dos arquivos fazem o programa retornar erro.

## Experimentos ponderados da Parte 2

O programa ponderado usa lista de adjacência e compara os dois Dijkstras sobre
as mesmas origens aleatórias. A semente padrão é `242`.

As origens são amostradas sem reposição pelo algoritmo de Floyd, usando O(k)
de espaço, e gravadas no CSV. Essa amostragem difere da versão anterior, que
embaralhava todos os V vértices. Para comparar versões ou plataformas, usar
as mesmas origens registradas; apenas repetir a semente não garante a mesma
amostra entre versões da biblioteca padrão.

```bash
./build/weighted_graph_experiments \
    data_parte2/grafo_1.txt \
    resultados/parte2/grafo_1 \
    100 \
    242 \
    both
```

Argumentos:

1. caminho do grafo ponderado;
2. prefixo dos arquivos de saída;
3. quantidade de origens, com padrão `100`;
4. semente aleatória, com padrão `242`;
5. modo `both`, `vector` ou `heap`, com padrão `both`;
6. opcional: CSV `order,source` para reutilizar exatamente uma amostra anterior.

Quando o CSV de origens é fornecido, ele define a amostra e sua ordem; a
semente fica apenas como metadado. A quantidade informada deve coincidir com
a quantidade de linhas, e origens repetidas ou fora do grafo são rejeitadas.
Por exemplo, para repetir as cinco origens históricas de W3:

```sh
./build/weighted_graph_experiments ../TP2/grafo_W_3.txt resultados/parte2/revisao_W3 5 242 both resultados/parte2/grafo_W_3_both_sources.csv
```

Arquivos produzidos:

- `<prefixo>_paths.csv`: distância e caminho de 10 para 20, 30, 40, 50 e 60;
- `<prefixo>_sources.csv`: amostra de origens, na ordem utilizada;
- `<prefixo>_benchmark.csv`: tempo total e médio das versões com vetor e heap.

Leitura, escrita e geração da amostra ficam fora do intervalo cronometrado. O
programa alterna qual implementação executa primeiro, interrompe antes das
medições quando encontra peso negativo e confere se os dois Dijkstras
produziram as mesmas distâncias.

Os testes usam verificações que continuam ativas em Release e com `NDEBUG`.
Um teste adicional do CTest confirma que uma falha proposital retorna erro.

Nos algoritmos, `neighbor_view` empresta os vizinhos da lista sem copiá-los;
na matriz, materializa a lista correspondente. A vista emprestada só permanece
válida enquanto o grafo existir e não for alterado. A interface `neighbors`
continua disponível para obter uma cópia independente.

A leitura de listas acumula as arestas e reduz duplicatas por vértice,
mantendo o menor peso e a ordem de primeira ocorrência dos vizinhos.
Isso evita a busca linear por duplicatas a cada linha: o custo de redução é
O(∑ d(v) log d(v)), com memória temporária O(Δ), contando as incidências de
entrada antes de remover duplicatas. A inserção individual por
`add_edge` continua disponível e procura duplicatas linearmente no grau.

O parser faz a conversão dos campos diretamente, sem criar um stream por linha.
A compactação das listas foi avaliada e não foi mantida, pois não reduziu a memória
residente no ensaio controlado. A mediana dos graus usa seleção
parcial, com custo médio linear, preservando a definição para tamanhos pares.

## Rede de pesquisadores

O estudo identifica os pesquisadores pela grafia exata do arquivo de nomes e
gera as distâncias e os caminhos com identificadores e nomes completos:

```bash
./build/researcher_experiments \
    data_parte2/rede_colaboracao.txt \
    data_parte2/rede_colaboracao_vertices.txt \
    resultados/parte2/rede_pesquisadores.csv
```

## Medições de manutenção

As ferramentas de perfil são opcionais e não fazem parte da suíte rápida:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGRAPH_STUDY_BUILD_BENCHMARKS=ON
cmake --build build --config Release
python3 benchmarks/run_profile.py build/graph_profile ../TP3/grafo_W_1.txt resultados/perfil_W1.csv --rounds 5 --sources 3 --mode search --timeout 120
python3 benchmarks/inventory.py build/graph_inventory resultados/inventory.csv ../TP3/grafo_W_1.txt ../TP3/grafo_W_2.txt --timeout 180
```

O perfil separa carga, estatísticas, BFS, DFS e Dijkstra, registra cada amostra e
confere uma assinatura dos resultados entre rodadas. `search` evita o Dijkstra
linear; `all` inclui essa versão e só deve ser usado em grafos pequenos. Em grafos
com pesos negativos, ambos os Dijkstras são omitidos. `matrix` também se destina
somente a grafos pequenos. O perfilador não estima a viabilidade dessas escolhas;
o prazo do script limita cada processo, e entradas grandes devem usar lista.

Use `--baseline <perfilador_preservado>` para alternar a ordem entre versões em
cada rodada e exigir as mesmas assinaturas de resultados entre ambas.

O perfil registra RSS e pico de RSS do processo em bytes no Linux, com zero quando
a plataforma não oferece essa coleta. Isso inclui a biblioteca, resultados e
runtime. A conferência dos resultados e a exportação do perfil ficam fora dos
tempos medidos. Cada rodada usa um processo novo; o cache de arquivos do sistema
pode ser compartilhado. Os arquivos JSON adjacentes registram comandos, hashes,
ambiente e estado. No inventário, duplicatas são contadas por par ordenado, sem
fundir arcos inversos; as estatísticas de pesos contam as linhas de entrada.

Resultados e limites da etapa 1: [registro da manutenção](docs/ETAPA1.md).
