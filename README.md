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

## Próximas etapas

- executar os experimentos nos grafos da disciplina e preencher o relatório.

## Compilação e testes

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

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

## Experimentos ponderados da Parte 2

O programa ponderado usa lista de adjacência e compara os dois Dijkstras sobre
as mesmas origens aleatórias. A semente padrão é `242`.

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
4. semente aleatória, com padrão `242`.
5. modo `both`, `vector` ou `heap`, com padrão `both`.

Arquivos produzidos:

- `<prefixo>_paths.csv`: distância e caminho de 10 para 20, 30, 40, 50 e 60;
- `<prefixo>_sources.csv`: amostra de origens, na ordem utilizada;
- `<prefixo>_benchmark.csv`: tempo total e médio das versões com vetor e heap.

Leitura, escrita e geração da amostra ficam fora do intervalo cronometrado. O
programa alterna qual implementação executa primeiro, interrompe antes das
medições quando encontra peso negativo e confere se os dois Dijkstras
produziram as mesmas distâncias.
