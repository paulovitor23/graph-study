# Etapa 2 — Grafos direcionados e caminhos mínimos

A etapa 2 foi concluída em 19/11/2026 sobre a revisão `1f9f4a08d5a6677b1967d4bb752013c5e74f14f8`, que continha a etapa 1. Foram implementados grafos direcionados em lista de adjacência, Bellman–Ford orientado ao destino e Floyd–Warshall entre todos os pares. As funcionalidades não direcionadas e as CLIs históricas foram preservadas. Os experimentos grandes e a produção das tabelas do TP3 pertencem à etapa 3.

## Direção e compatibilidade

`GraphDirection::Undirected` é o padrão nos construtores, em `make_graph` e em `read_graph_from_file`. Para carregar um arquivo direcionado, fornecer `GraphDirection::Directed` como terceiro argumento do leitor. A numeração dos arquivos continua começando em 1; os identificadores internos continuam começando em 0.

No modo direcionado, inserir `u → v` não insere `v → u`. Duplicatas são reduzidas por par ordenado, mantendo o menor peso e a ordem da primeira ocorrência. Cada arco é contabilizado, inclusive quando o primeiro identificador é maior que o segundo. Laços são armazenados uma vez. `has_negative_weight` considera todos os arcos armazenados.

BFS, DFS, distância por BFS e os dois Dijkstras já percorriam `neighbor_view`. A mudança da representação faz essas operações respeitarem as arestas de saída sem duplicar as implementações. Dijkstra mantém sua rejeição de pesos negativos em qualquer componente. `degree` e as estatísticas de um grafo direcionado passam a significar grau de saída.

Componentes conexas e diâmetro não direcionados rejeitam grafos direcionados com `domain_error`. A matriz direcionada, opcional no trabalho, é rejeitada com `invalid_argument` antes da alocação; a matriz não direcionada continua funcionando.

## Bellman–Ford até um destino

`bellman_ford_to_target(graph, target)` retorna `BellmanFordResult`:

- `distance[u]`: distância de `u` até `target`;
- `next[u]`: próximo vértice da árvore orientada ao destino;
- `path_from(u)`: caminho reconstruído;
- `has_negative_cycle`: presença de ciclo negativo em qualquer componente;
- `status(u)`: classificação da consulta;
- `relaxation_passes` e `examined_edges`: contadores da busca até o destino, excluindo o preparo e as verificações de ciclos.

As duas melhorias da aula 14 foram incorporadas. A memória não guarda uma matriz por número de arestas: usa um vetor de melhores distâncias e um de sucessores. Para a melhoria de tempo, são construídas listas de predecessores; quando a distância de `w` muda, só são examinados arcos `v → w` que possam melhorar os predecessores. Duas fronteiras separam os passos, e as ativações são deduplicadas. A fronteira vazia encerra a busca. As distâncias são atualizadas no próprio vetor, podendo acelerar a propagação dentro de um passo; a comparação com a referência por rodadas verifica essa política.

A busca executa no máximo `V−1` passos. A verificação adicional identifica melhorias que ainda seriam possíveis e propaga a marcação aos vértices que podem chegar ao ciclo e depois ao destino. Esses vértices recebem `−∞` e sucessor inválido.

Se a busca não identificou um ciclo que afete o destino e existem pesos negativos, uma detecção complementar começa com distância zero em todos os vértices, equivalente a uma superorigem virtual. Assim, um ciclo em outra componente também é informado, sem marcar consultas que ele não afeta. Quando não existem pesos negativos, essa detecção é dispensada. Todo esse preparo e detecção integra o custo da chamada; os contadores da busca não representam o trabalho total.

Memória: `O(V+E)`, incluindo as listas temporárias de predecessores. Tempo de pior caso: `O(VE)`; as otimizações não alteram esse limite. Cada chamada constrói suas estruturas auxiliares e não reutiliza soluções de chamadas anteriores.

## Floyd–Warshall e árvores por origem

`floyd_warshall(graph, memory_budget_bytes)` retorna `FloydWarshallResult`. Distâncias e predecessores ficam em vetores contíguos no índice `source * vertex_count + target`. As consultas disponíveis são `distance_between`, `status`, `path` e `predecessors_from`.

A implementação usa uma única matriz de distâncias, com `k` como laço externo. Não guarda o histórico tridimensional. Em uma melhoria finita via `k`, atualiza `pred(i,j) = pred(k,j)`. As árvores por origem já estão codificadas nas linhas da matriz de predecessores; apenas a linha solicitada é copiada por `predecessors_from`.

A diagonal começa em zero, mas um laço negativo prevalece sobre esse valor. Diagonais negativas identificam ciclos; um ciclo já conhecido propaga `−∞` pelos pares que o atravessam, evitando crescimento numérico desnecessário. A classificação final marca cada par que alcança um ciclo negativo e pode seguir dele até o destino. As consultas finitas de outras regiões continuam válidas.

Tempo: `Θ(V³)`. Memória: `Θ(V²)`, incluindo as duas matrizes e uma lista linear de índices de ciclos.

`floyd_warshall_memory_bytes(V)` verifica o quadrado, os bytes e os limites dos vetores antes da alocação. O orçamento padrão é 256 MiB. A estimativa é

`V² × (sizeof(double) + sizeof(size_t)) + V × sizeof(size_t)`.

Ela exclui o grafo, os objetos dos vetores e overhead do alocador. A chamada lança `length_error` se a estimativa exceder o orçamento. O runner da etapa 3 deverá limitar esse orçamento à memória disponível, reservar margem para o processo e aplicar também o prazo de execução. Usar lista de adjacência para o grafo não elimina as matrizes internas do algoritmo.

## Estados e limites numéricos

| Situação da consulta | Estado | Distância | Caminho |
| --- | --- | --- | --- |
| Mínimo finito | `Finite` | `double` finito | Vértices da origem ao destino |
| Destino inalcançável | `Unreachable` | `+∞` | Vetor vazio |
| Influência de ciclo negativo | `NegativeCycle` | `−∞` | `domain_error` |

Os dois resultados usam o mesmo enum `PathStatus`. O indicador global de ciclo não substitui a classificação por consulta. Consultar índices inexistentes lança `out_of_range`; sucessores e predecessores sem caminho finito usam o sentinela `no_vertex`.

O relaxamento usa comparações estritas de `double`, sem epsilon. Empates podem gerar caminhos diferentes com o mesmo custo. Os testes comparam custos reais com tolerância, sem modificar a definição do relaxamento. Somas intermediárias finitas fora do intervalo de `double` lançam `overflow_error`; esse erro numérico não é convertido silenciosamente em ausência de caminho ou ciclo negativo.

## Testes e evidências

O CTest aprovou os cinco testes: suíte histórica, suíte direcionada, falha proposital para comprovar verificações ativas com `NDEBUG`, CLIs e inventário. A suíte direcionada foi executada também com AddressSanitizer e UndefinedBehaviorSanitizer, sem diagnóstico de erro.

Os testes incluem:

- Direção de inserção e leitura, arcos opostos independentes, duplicatas, ordem de vizinhos, laços e arco negativo com identificador inicial maior que o final.
- BFS/DFS assimétricas, pais, níveis, distâncias e equivalência dos Dijkstras em pesos não negativos.
- Distâncias e caminhos conhecidos até um destino, negativos sem ciclo, alvo isolado, ciclos positivos e de peso zero, caminho com exatamente `V−1` arestas e ciclo negativo detectado no passo adicional.
- Ciclos negativos que afetam a consulta, que não alcançam o destino ou que estão em outra componente; laços negativos e grafos não direcionados com aresta negativa.
- Conferência da parada antecipada e do processamento seletivo pelos contadores da busca.
- Floyd–Warshall com predecessores, diagonais, pares afetados e grafos vazios; propagação de ciclos em grafo denso e laço negativo com peso de grande magnitude.
- Rejeição de orçamento antes de percorrer vizinhos, usando um grafo de formato simulado com 25.000 vértices, sem alocar uma matriz grande; overflow de estimativa e de somas de custos.

Foram conferidos **38.340 pares** em **1.460 grafos pequenos**:

- 1.280 grafos gerados com semente fixa, com e sem pesos negativos, contra Bellman–Ford independente por origem, usando cópia da rodada anterior. Sem pesos negativos, os dois Dijkstras também foram confrontados com essa referência. As reconstruções finitas tiveram suas arestas e custos conferidos.
- 180 DAGs com pesos positivos e negativos, ordem de inserção embaralhada, conferidos por enumeração de caminhos e Floyd–Warshall de referência por etapas.

As saídas funcionais das três CLIs históricas coincidiram com os executáveis preservados, incluindo árvores, componentes, caminhos e nomes. Os campos de tempo, memória e localização da saída são excluídos dessa comparação. A verificação está em [final_checks.log](../../TP3/etapa2/final_checks.log).

## Reprodução e escopo restante

Ambiente validado: GCC 13.3.0 no WSL2 x86-64. Release usou `-std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic`. A execução com sanitizadores usou `-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer`.

```sh
cmake -S . -B build/etapa2 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-DNDEBUG -DCMAKE_CXX_FLAGS_RELEASE=-O2 -DGRAPH_STUDY_BUILD_BENCHMARKS=ON
cmake --build build/etapa2 -j2
ctest --test-dir build/etapa2 --output-on-failure
./build/etapa2/directed_graph_tests
```

O [README](../README.md) mostra o uso das APIs. O manifesto, os hashes e uma cópia dos fontes estão em [TP3/etapa2](../../TP3/etapa2/). A compilação nativa com MSVC não foi executada.

Não foram iniciados os experimentos grandes nem calculadas as respostas dos cinco grafos. A etapa 3 deverá criar o runner e as tabelas, executar as dez rodadas quando viáveis e registrar os casos pulados ou abortados por memória ou previsão de muitas horas, conforme a política do usuário.
