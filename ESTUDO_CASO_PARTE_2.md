# Estudo de caso - Trabalho Prático 2

## Ambiente e metodologia

Os experimentos foram executados no macOS 26.5.2, arquitetura Apple Silicon
arm64, com Apple Clang 21.0.0 e as opções `-O2 -DNDEBUG`. Todos os grafos foram
carregados como listas de adjacência. A semente aleatória foi `242`.

O cronômetro inclui somente a execução do Dijkstra. Leitura do arquivo,
seleção das origens e escrita dos resultados ficam fora da medição. Quando as
duas versões são executadas juntas, a ordem entre vetor e heap é alternada para
reduzir o viés de cache. As duas versões usam exatamente as mesmas origens.

Os arquivos possuem arestas repetidas com pesos diferentes. A biblioteca
mantém o menor peso para cada par, pois pesos maiores entre os mesmos vértices
nunca melhoram um caminho mínimo. Todos os pesos encontrados estão no intervalo
de 0 a 10; não há pesos negativos.

## Características dos grafos

| Grafo | Vértices | Linhas de aresta | Arestas únicas | Pesos negativos |
|---|---:|---:|---:|---:|
| W3 | 100.000 | 6.099.941 | 6.099.868 | 0 |
| W4 | 1.000.000 | 30.999.978 | 30.999.944 | 0 |
| W5 | 10.000.000 | 39.999.995 | 39.999.991 | 0 |
| W6 | 10.000.000 | 73.403.372 | 73.403.372 | 0 |
| W7 | 50.000.000 | 59.999.999 | 59.999.999 | 0 |

## Distâncias solicitadas

| Grafo | d(10,20) | d(10,30) | d(10,40) | d(10,50) | d(10,60) |
|---|---:|---:|---:|---:|---:|
| W3 | 0,80 | 0,89 | 0,84 | 0,87 | 0,97 |
| W4 | 2,66 | 2,02 | 2,52 | 2,69 | 2,41 |
| W5 | 17,34 | 17,24 | 17,37 | 15,43 | 17,99 |
| W6 | 11,26 | 10,67 | 11,31 | 9,60 | 10,72 |
| W7 | 93,47 | 95,08 | 100,69 | 95,77 | 82,25 |

Os caminhos completos estão nos arquivos `grafo_W_N_*_paths.csv`. Todos os
25 pares solicitados possuem caminho.

## Desempenho

| Grafo | Algoritmo | Origens | Tempo total (ms) | Média (ms) |
|---|---|---:|---:|---:|
| W3 | Vetor | 5 | 82.420,070 | 16.484,014 |
| W3 | Heap, mesma amostra | 5 | 430,130 | 86,026 |
| W3 | Heap, rodada final | 100 | 8.986,991 | 89,870 |
| W4 | Vetor | - | inviável | inviável |
| W4 | Heap | 100 | 107.306,018 | 1.073,060 |
| W5 | Vetor | - | inviável | inviável |
| W5 | Heap | 10 | 106.956,979 | 10.695,698 |
| W6 | Vetor | - | inviável | inviável |
| W6 | Heap | 10 | 133.392,709 | 13.339,271 |
| W7 | Vetor | - | inviável | inviável |
| W7 | Heap | 5 | 628.095,525 | 125.619,105 |

No W3, usando as mesmas cinco origens, o heap foi aproximadamente 191,6 vezes
mais rápido que o vetor. A versão linear levou em média 16,48 segundos por
origem, enquanto o heap levou 86 milissegundos.

## Viabilidade da versão com vetor

A seleção linear custa `O(V²)`. O W3 já exige aproximadamente 16,48 segundos
por origem. Usando apenas a razão quadrática do número de vértices como uma
estimativa de ordem de grandeza, e não como tempo medido:

- W4: cerca de 27,5 minutos por origem;
- W5 e W6: cerca de 45,8 horas por origem;
- W7: cerca de 47,7 dias por origem.

Por esse motivo, a versão com vetor não foi executada nos grafos W4 a W7. Os
valores acima são extrapolações de complexidade e não devem ser apresentados
como medições. Para o heap foram usadas 100 origens nos grafos W3 e W4, 10
origens nos grafos W5 e W6 e 5 origens no W7. O tamanho amostral foi reduzido
nos maiores grafos porque cada execução no W7 levou cerca de dois minutos na
rodada final.

## Conclusões

O heap indexado tornou possível processar os cinco grafos. O tempo aumentou
com o número de vértices e arestas, mas permaneceu muito abaixo do custo
quadrático da versão linear. O W6 foi mais lento que o W5 apesar de ambos terem
10 milhões de vértices, coerente com o W6 possuir aproximadamente 83% mais
arestas únicas. O W7 apresentou o maior custo devido aos 50 milhões de
vértices e às estruturas auxiliares de distância, pais, heap e posições.

O estudo da rede de pesquisadores ainda não foi executado porque nenhum dos
arquivos anexados contém nomes ou uma tabela de correspondência entre nomes e
índices. Esse arquivo continua necessário para responder à última parte do
enunciado sem inventar identificadores.

## Arquivos consolidados

- `resultados/parte2/resumo_distancias.csv`;
- `resultados/parte2/resumo_desempenho.csv`;
- arquivos individuais de caminhos, origens e benchmark em
  `resultados/parte2/`.
