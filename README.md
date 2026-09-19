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
- distância entre dois vértices usando BFS.

## Próximas etapas

- busca em profundidade (DFS), incluindo pai e nível dos vértices;
- componentes conexas;
- distâncias e diâmetro;
- arquivos de saída e medições dos estudos de caso.

## Compilação e testes

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
