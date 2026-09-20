# graph-study

Biblioteca em C++17 para manipulação de grafos não direcionados, desenvolvida
para o trabalho de Teoria dos Grafos.

## Funcionalidades implementadas

- representação por lista de adjacência;
- representação por matriz de adjacência;
- leitura do grafo a partir de arquivo texto;
- número de vértices e arestas;
- grau de cada vértice;
- grau mínimo, máximo, médio e mediana dos graus.
- busca em largura (BFS), com pai, nível e ordem de visita;
- busca em profundidade (DFS), com pai, nível e ordem de visita;
- componentes conexas, ordenadas por tamanho decrescente;
- distância entre dois vértices usando BFS;
- diâmetro exato;
- aproximação do diâmetro para grafos grandes.

## Próximas etapas

- executar os experimentos nos grafos da disciplina e preencher o relatório.

## Compilação e testes

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Programa de experimentos

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
