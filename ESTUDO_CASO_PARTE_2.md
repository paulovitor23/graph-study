# Estudo de caso - Trabalho Prático 2

## Ambiente e metodologia

As medições históricas abaixo, anteriores às melhorias desta revisão, foram
executadas no macOS 26.5.2, arquitetura Apple Silicon
arm64, com Apple Clang 21.0.0 e as opções `-O2 -DNDEBUG`. Todos os grafos foram
carregados como listas de adjacência. A semente aleatória foi `242`.

O cronômetro inclui somente a execução do Dijkstra. Leitura do arquivo,
seleção das origens e escrita dos resultados ficam fora da medição. Quando as
duas versões são executadas juntas, a ordem entre vetor e heap é alternada para
reduzir o viés de cache. As duas versões usam exatamente as mesmas origens.

Os arquivos possuem arestas repetidas com pesos diferentes. A biblioteca
mantém o menor peso para cada par, pois pesos maiores entre os mesmos vértices
nunca melhoram um caminho mínimo. Nos grafos W3–W7, os pesos encontrados estão
no intervalo de 0 a 10; não há pesos negativos. Na rede de pesquisadores,
os pesos variam de aproximadamente 0,003735 a 113, também sem negativos.

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

Todos os 25 pares solicitados possuem caminho. A tabela abaixo inclui os
caminhos completos, com a numeração original dos vértices.

| Grafo | Origem | Destino | Distância | Caminho mínimo |
|---|---:|---:|---:|---|
| W3 | 10 | 20 | 0,80 | 10 → 2858 → 35293 → 3905 → 24445 → 39345 → 61279 → 41321 → 70540 → 26049 → 84450 → 68904 → 87862 → 86778 → 27694 → 16608 → 86355 → 15760 → 20 |
| W3 | 10 | 30 | 0,89 | 10 → 2858 → 35293 → 96094 → 85187 → 53177 → 64898 → 87087 → 77935 → 42304 → 78071 → 93985 → 27642 → 26151 → 30 |
| W3 | 10 | 40 | 0,84 | 10 → 2858 → 35293 → 3905 → 24445 → 86286 → 14327 → 19697 → 15930 → 46307 → 40 |
| W3 | 10 | 50 | 0,87 | 10 → 56663 → 84437 → 19040 → 82510 → 17622 → 94992 → 70948 → 19878 → 50 |
| W3 | 10 | 60 | 0,97 | 10 → 2858 → 35293 → 3905 → 24445 → 89584 → 2874 → 99253 → 66532 → 33719 → 96654 → 75728 → 79865 → 74717 → 59399 → 8718 → 60 |
| W4 | 10 | 20 | 2,66 | 10 → 532245 → 877016 → 554617 → 430107 → 437166 → 805646 → 841402 → 663364 → 655748 → 634947 → 318070 → 807260 → 20 |
| W4 | 10 | 30 | 2,02 | 10 → 873980 → 182382 → 261305 → 81769 → 694525 → 219466 → 207765 → 44636 → 183747 → 920911 → 35338 → 239369 → 417194 → 132461 → 740528 → 707581 → 30 |
| W4 | 10 | 40 | 2,52 | 10 → 873980 → 182382 → 674971 → 557022 → 188455 → 418550 → 275098 → 277633 → 801708 → 730863 → 274827 → 386086 → 188007 → 477496 → 40 |
| W4 | 10 | 50 | 2,69 | 10 → 873980 → 182382 → 261305 → 81769 → 694525 → 382144 → 748624 → 663332 → 149827 → 349584 → 735867 → 9905 → 716970 → 472621 → 407659 → 994210 → 630598 → 50 |
| W4 | 10 | 60 | 2,41 | 10 → 873980 → 182382 → 674971 → 106317 → 27985 → 280367 → 227336 → 364950 → 883474 → 283156 → 161217 → 224210 → 188021 → 614364 → 358029 → 855615 → 822462 → 774634 → 60 |
| W5 | 10 | 20 | 17,34 | 10 → 7622850 → 4615833 → 6528266 → 1295458 → 7103730 → 8790605 → 7788327 → 6766415 → 7155379 → 2031914 → 3908338 → 8885874 → 20 |
| W5 | 10 | 30 | 17,24 | 10 → 2846336 → 5413988 → 202621 → 9270546 → 6836812 → 2983314 → 8775277 → 5549785 → 3485759 → 4341576 → 8868678 → 3166991 → 5117196 → 7291082 → 1816279 → 5664093 → 8947758 → 30 |
| W5 | 10 | 40 | 17,37 | 10 → 1299954 → 1419794 → 9885733 → 5641294 → 409605 → 4105715 → 660832 → 5850281 → 3215052 → 7676193 → 9008138 → 4846487 → 9798152 → 40 |
| W5 | 10 | 50 | 15,43 | 10 → 1299954 → 9927727 → 8451042 → 5368621 → 4276332 → 3716282 → 3724752 → 700154 → 9271362 → 6416250 → 396789 → 50 |
| W5 | 10 | 60 | 17,99 | 10 → 1299954 → 1419794 → 1899178 → 7524819 → 757290 → 9445351 → 9611705 → 9104671 → 8941517 → 264365 → 3945175 → 3090234 → 2071841 → 8424191 → 2860740 → 1175531 → 1925405 → 7718765 → 3193827 → 7824884 → 1454010 → 60 |
| W6 | 10 | 20 | 11,26 | 10 → 7194590 → 1623776 → 6004729 → 5713792 → 8358296 → 2533780 → 5227148 → 8652974 → 6001519 → 3007875 → 2659882 → 1975860 → 7178227 → 4239765 → 4333977 → 5135142 → 7805172 → 6937097 → 2042733 → 20 |
| W6 | 10 | 30 | 10,67 | 10 → 6473892 → 2441538 → 1548272 → 787618 → 5728075 → 8108429 → 6007822 → 5550988 → 2523255 → 782814 → 7306955 → 153130 → 9107012 → 8930108 → 2199545 → 9604404 → 5436447 → 8094054 → 8301086 → 7882916 → 7776260 → 30 |
| W6 | 10 | 40 | 11,31 | 10 → 7194590 → 1308444 → 2430454 → 9006783 → 9339181 → 6729573 → 6733497 → 5619464 → 9959710 → 9649921 → 40 |
| W6 | 10 | 50 | 9,60 | 10 → 6473892 → 2441538 → 3496708 → 8719217 → 3608871 → 1734087 → 8196874 → 3353190 → 4171489 → 6448393 → 8854015 → 50 |
| W6 | 10 | 60 | 10,72 | 10 → 6473892 → 2441538 → 1548272 → 787618 → 9566932 → 345742 → 218396 → 8462300 → 2279724 → 9337595 → 9606830 → 9928328 → 8701739 → 8354591 → 8135590 → 4215828 → 8346899 → 3411658 → 6447603 → 60 |
| W7 | 10 | 20 | 93,47 | 10 → 44348966 → 895015 → 41433396 → 36342079 → 15296633 → 26159915 → 16943639 → 38743389 → 10078159 → 21570419 → 11797967 → 14033688 → 15395825 → 37638078 → 10256175 → 47812933 → 16114395 → 3709139 → 16167977 → 42063355 → 25051663 → 11357393 → 1237413 → 44073100 → 46623569 → 16564439 → 33972685 → 44128375 → 38865462 → 44580311 → 49167593 → 34107790 → 20 |
| W7 | 10 | 30 | 95,08 | 10 → 44348966 → 895015 → 41433396 → 49215936 → 31738577 → 6483504 → 6934370 → 37879154 → 8001897 → 22183730 → 7265763 → 1661582 → 11612335 → 1574885 → 30723220 → 39141968 → 27645810 → 37795681 → 33920812 → 34023581 → 12510615 → 18298407 → 31754868 → 14520632 → 7334968 → 721843 → 7608283 → 44662249 → 46198809 → 161794 → 37094018 → 30 |
| W7 | 10 | 40 | 100,69 | 10 → 44348966 → 895015 → 41433396 → 49215936 → 31738577 → 6483504 → 35576121 → 13084740 → 8200352 → 334647 → 21020208 → 33240812 → 43511183 → 39829015 → 31993368 → 35257514 → 1449696 → 2909571 → 35131499 → 46634817 → 8774626 → 26615224 → 12496865 → 25597904 → 17624904 → 28515031 → 9039078 → 42335617 → 44883250 → 25945871 → 4357404 → 43686238 → 2293704 → 18096524 → 17747192 → 14433007 → 4134285 → 11543624 → 49518359 → 34048991 → 36125121 → 20450161 → 31315434 → 44798389 → 37766241 → 40 |
| W7 | 10 | 50 | 95,77 | 10 → 44348966 → 895015 → 41433396 → 30012782 → 5306752 → 45425007 → 45928549 → 3090492 → 35262022 → 25302788 → 39964857 → 25438408 → 42769429 → 18259600 → 14013095 → 21407931 → 29028767 → 13632589 → 26269428 → 19841425 → 16775149 → 408668 → 33679805 → 18032133 → 15314852 → 41061763 → 21731902 → 43132195 → 43482012 → 248139 → 24050455 → 39437117 → 4560936 → 50 |
| W7 | 10 | 60 | 82,25 | 10 → 44348966 → 895015 → 41433396 → 36342079 → 15296633 → 26159915 → 27614435 → 30636628 → 46606379 → 39970228 → 44691860 → 28472827 → 14457897 → 49527742 → 23832529 → 33803765 → 40354942 → 41967334 → 30441838 → 2384620 → 9629521 → 15597152 → 8376037 → 19638652 → 35026345 → 21270637 → 16840822 → 36015359 → 4125149 → 20825658 → 31193177 → 35236066 → 37175171 → 16297527 → 34154631 → 60 |

## Desempenho

| Grafo | Algoritmo | Origens | Tempo total (ms) | Média (ms) |
|---|---|---:|---:|---:|
| W3 | Vetor | 5 | 82.420,070 | 16.484,014 |
| W3 | Heap, mesma amostra | 5 | 430,130 | 86,026 |
| W3 | Heap, rodada final | 100 | 8.986,991 | 89,870 |
| W4 | Vetor | - | não executado (custo estimado elevado) | — |
| W4 | Heap | 100 | 107.306,018 | 1.073,060 |
| W5 | Vetor | - | não executado (custo estimado elevado) | — |
| W5 | Heap | 10 | 106.956,979 | 10.695,698 |
| W6 | Vetor | - | não executado (custo estimado elevado) | — |
| W6 | Heap | 10 | 133.392,709 | 13.339,271 |
| W7 | Vetor | - | não executado (custo estimado elevado) | — |
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

Nas medições históricas, a versão com vetor não foi executada em W4–W7. Os
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

## Rede de colaboração entre pesquisadores

A rede possui 722.385 pesquisadores. Os nomes foram procurados pela grafia
exata exigida no enunciado, e o Dijkstra com heap foi executado uma única vez
a partir de Edsger W. Dijkstra. O peso representa proximidade inversamente
proporcional à quantidade de artigos em coautoria; portanto, o caminho mínimo
favorece relações de colaboração mais próximas.

| Destino | ID | Distância | Caminho mínimo |
|---|---:|---:|---|
| Alan M. Turing | 11.365 | sem caminho | componente desconectada |
| J. B. Kruskal | 471.365 | 3,480368 | Edsger W. Dijkstra → John R. Rice → Dan C. Marinescu → Howard Jay Siegel → Edwin K. P. Chong → Ness B. Shroff → R. Srikant → Albert G. Greenberg → J. B. Kruskal |
| Jon M. Kleinberg | 5.709 | 2,706994 | Edsger W. Dijkstra → A. J. M. van Gasteren → Gerard Tel → Hans L. Bodlaender → Dimitrios M. Thilikos → Prabhakar Ragde → Avi Wigderson → Eli Upfal → Prabhakar Raghavan → Jon M. Kleinberg |
| Éva Tardos | 11.386 | 2,753514 | Edsger W. Dijkstra → A. J. M. van Gasteren → Gerard Tel → Hans L. Bodlaender → Jan van Leeuwen → Mark H. Overmars → Micha Sharir → Haim Kaplan → Robert Endre Tarjan → Andrew V. Goldberg → Serge A. Plotkin → Éva Tardos |
| Daniel R. Figueiredo | 343.930 | 2,942831 | Edsger W. Dijkstra → John R. Rice → Dan C. Marinescu → Chuang Lin → Bo Li → Y. Thomas Hou → Zhi-Li Zhang → Donald F. Towsley → Daniel R. Figueiredo |

Alan M. Turing está em uma componente formada somente por ele e pelo vértice
717.737, ligados por uma aresta de peso 1. Por isso, não existe caminho entre
Edsger W. Dijkstra e Alan M. Turing nessa base.

## Arquivos consolidados

- `resultados/parte2/resumo_distancias.csv`;
- `resultados/parte2/resumo_desempenho.csv`;
- `resultados/parte2/rede_pesquisadores.csv`;
- arquivos individuais de caminhos, origens e benchmark em
  `resultados/parte2/`.


## Reprodutibilidade e estado após a revisão

A tabela de desempenho acima preserva as medições históricas da versão
`6b8e0df`. O modelo exato do processador Apple e a RAM não foram registrados
nesses arquivos; precisam ser informados pelo responsável pela execução.
As opções `-O2 -DNDEBUG` foram declaradas no estudo original, mas o comando
completo da execução histórica não foi preservado. Esses dados não foram
inferidos a partir desta máquina Windows.

A versão revisada mantém verificações de testes mesmo com `NDEBUG`, usa
vistas de adjacência para evitar cópias, carrega listas em lote com redução de
duplicatas e preservação da ordem de vizinhos, e amostra origens pelo algoritmo
de Floyd com espaço O(k). A nova amostragem pode produzir origens diferentes
das antigas mesmo com semente 242. Comparações devem usar os mesmos arquivos
de origens; não se deve atribuir à nova implementação os tempos históricos.

Para reproduzir novos experimentos em um compilador GCC/Clang:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/weighted_graph_experiments ../TP2/grafo_W_3.txt resultados/parte2/revisao_W3 5 242 both
```

Em geradores com várias configurações, os executáveis podem ficar em
`build/Release`. Registrar compilador, sistema, CPU, memória, opções, versão
do código e o CSV de origens junto de cada medição. A revisão local e seus
limites estão documentados em `../TP2/revisao/03-melhorias-aplicadas.md`.

A rodada de uma hora acrescentou uma medição exploratória em W4.
Permanecem pendentes as versões com vetor de W5–W7 e amostras adicionais de W4. Estimativas quadráticas não substituem médias medidas. O relatório
final em LaTeX depende de completar essa comparação ou de uma orientação do
professor que aceite explicitamente a limitação.

## Piloto local da versão revisada

Em 02/11/2026, a versão histórica `6b8e0df` e a versão revisada foram
compiladas com GCC 13.3.0 e `-std=c++17 -O2 -DNDEBUG`, no Ubuntu via WSL2
x86-64. A CPU é AMD Ryzen 5 5625U; o Windows informa aproximadamente
15,41 GiB de memória física utilizável, e o WSL enxerga 7,46 GiB.

Em W3 foram reutilizadas exatamente as cinco origens históricas, na mesma
ordem: 23.419, 2.484, 2.311, 90.835 e 85.090. A ordem entre vetor e heap foi
alternada por origem. A carga ficou fora dos tempos de Dijkstra.

| Medida em W3 | Versão anterior (ms) | Versão revisada (ms) |
|---|---:|---:|
| Carga, uma execução | 11.044,580 | 9.319,384 |
| Dijkstra com vetor, média de 5 origens | 21.008,284 | 22.907,173 |
| Dijkstra com heap, média de 5 origens | 171,152 | 146,348 |

Neste piloto, o tempo médio do heap diminuiu cerca de 14,5%, enquanto o
vetor ficou aproximadamente 9,0% mais lento. A eliminação das cópias não
altera o custo quadrático da seleção linear. As versões foram executadas
em sequência, sem múltiplas rodadas independentes; cache, frequência da CPU
e carga do sistema podem afetar os valores. Não se conclui um ganho geral
nem se atribui causalmente toda variação às alterações de código.

Os testes funcionais e 4.680 verificações contra Floyd–Warshall passaram.
Uma execução do programa de experimentos com heap também preservou os cinco
caminhos publicados de W3; o programa da rede preservou as distâncias,
identificadores e caminhos dos pesquisadores. Os tempos desta seção são
medições locais e não substituem os dados históricos do Mac.

Em W4, uma tentativa local a partir da primeira origem histórica, 416.635,
foi interrompida pelo limite total de 120 segundos, incluindo a carga de
aproximadamente 60,6 segundos. O algoritmo linear começou, mas não concluiu
antes do limite. Não há média dessa tentativa; ela não prova inviabilidade
absoluta. W5–W7 não receberam novas execuções da versão linear.

## Rodada local de até uma hora

Janela autorizada: 02/11/2026, 18:53:49–19:53:49 (São Paulo). A sequência terminou em 02/11/2026 19:50:42. As versões foram executadas sequencialmente.

| Grafo | Algoritmo | Origens concluídas | Tempo total (ms) | Média (ms) | Estado |
|---|---|---:|---:|---:|---|
| W4 | vector | 1 | 2.477.118,865 | 2.477.118,865 | complete |
| W4 | heap | 1 | 3.290,068 | 3.290,068 | complete |
| W3 | vector | 5 | 117.216,501 | 23.443,300 | complete |
| W3 | heap | 5 | 730,904 | 146,181 | complete |
| W5 | heap | 3 | 79.492,191 | 26.497,397 | complete |
| W6 | heap | 3 | 93.460,078 | 31.153,359 | complete |

A origem única de W4 foi 416.635 e o heap executou antes do vetor. Em W3 a ordem foi alternada entre as cinco origens históricas. W5 e W6 usam prefixos das amostras históricas, com até três origens. Leitura, validação, escrita e a busca adicional para gerar caminhos ficam fora do tempo dos algoritmos.

Uma origem em W4 fornece uma medição exploratória, com desvio amostral indefinido. Os resultados de W3/W5/W6 também têm amostras pequenas. As versões lineares de W5/W6 e todos os novos testes de W7 ficaram fora desta janela. Execuções interrompidas não fornecem tempo para a busca interrompida; o CSV identifica separadamente buscas completas.

Os registros completos estão em `../TP2/medicoes_2026-11-02_185349/`: tempos individuais, eventos, origens, caminhos, recursos, hashes dos grafos, ambiente e cópia do código. O relatório final em LaTeX continua dependendo de resolver a comparação linear em W5–W7 ou de orientação do professor.

## Rodada suplementar de W7

W7 foi executado com heap e cinco origens entre 02/11/2026 23:09:18 e 02/11/2026 23:39:15, horário de São Paulo. Esta rodada é adicional à janela original de uma hora.

Tempo médio: **287.219,100 ms**; total: 1.436.095,501 ms; desvio-padrão amostral: 30.067,406 ms.

A carga levou 212.303,182 ms e ficou fora do cronômetro dos algoritmos. As origens foram [16145275, 6293940, 6555911, 33508449, 33183393], reutilizando o CSV histórico. Foi usado o mesmo executável da rodada anterior, verificado por SHA-256.

As cinco distâncias e os caminhos gerados a partir de 10 coincidiram com os históricos. A geração dos caminhos usou uma busca adicional, fora do benchmark. Agora os 25 caminhos dos cinco grafos foram revalidados com a versão revisada. As versões com vetor de W5–W7 continuam sem medição nesta versão.

Os dados de W7 estão em `../TP2/medicoes_2026-11-02_185349/W7/`. A consolidação das duas rodadas está em `summary_all_graphs.csv`, na pasta pai.
