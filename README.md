# Estruturas de Dados: Compactador de Huffman, SAT Solver e Grafos

Repositório contendo implementações práticas de algoritmos clássicos de compressão de dados, solucionadores lógicos e caminhos em grafos, desenvolvidos em linguagem C.

---

## 📁 Estrutura do Repositório

| Arquivo | Descrição |
| :--- | :--- |
| `huffman.c` | Algoritmo de compressão e descompressão de Huffman, com manipulação de bits em baixo nível e reconstrução de árvore em pré-ordem. |
| `sat_solver.c` | Solucionador para o problema da Satisfatibilidade Booleana (SAT) com suporte ao formato DIMACS CNF. |
| `arquivo.cnf` | Instância de teste no formato padrão DIMACS CNF para validação do SAT Solver. |
| `dijkstra.c` | Algoritmo de Dijkstra para cálculo do caminho mínimo em grafos ponderados (pesos não-negativos). |
| `kruskal.c` | Algoritmo de Kruskal para geração da Árvore Geradora Mínima (MST) utilizando estrutura *Union-Find*. |
| `Algoritmo de Dijkstra e Kruskal_ofc.pdf` | Documentação teórica, análise de complexidade e comparativo dos algoritmos de grafos. |

---

## 🚀 Como Compilar e Executar

Todos os códigos foram projetados para compilação nativa usando o GCC no Linux. Abaixo estão as instruções passo a passo para cada um deles.

### 1. Compactador de Huffman (`huffman.c`)

O programa suporta duas formas de uso: via argumentos na linha de comando (CLI) ou através de um menu interativo.

**Compilação:**

```bash
gcc -Wall -Wextra huffman.c -o huffman
```

**Uso via linha de comando:**

```bash
# Compactar um arquivo
./huffman -c arquivo_original.ext arquivo_comprimido.huff

# Descompactar um arquivo
./huffman -d arquivo_comprimido.huff arquivo_restaurado.ext
```

**Uso via menu interativo:**

```bash
./huffman
```

**Validação da integridade:**

Para confirmar que a descompressão restaurou o arquivo original sem perdas:

```bash
# Se não retornar nada, os arquivos são idênticos
diff arquivo_original.ext arquivo_restaurado.ext

# Os hashes gerados devem ser exatamente iguais
md5sum arquivo_original.ext arquivo_restaurado.ext
```

### 2. SAT Solver (`sat_solver.c`)

Recebe uma fórmula proposicional descrita em um arquivo CNF e determina se ela é Satisfatível (SAT) ou Insatisfatível (UNSAT).

**Compilação (com otimização `-O3`):**

```bash
gcc -O3 -Wall -Wextra sat_solver.c -o sat_solver
```

**Execução:**

```bash
./sat_solver arquivo.cnf
```

### 3. Algoritmos em Grafos

**Algoritmo de Dijkstra (Caminho Mínimo):**

```bash
gcc -Wall -Wextra dijkstra.c -o dijkstra
./dijkstra
```

**Algoritmo de Kruskal (Árvore Geradora Mínima):**

```bash
gcc -Wall -Wextra kruskal.c -o kruskal
./kruskal
```

---

## 🛠️ Tecnologias e Ferramentas Utilizadas

- **Linguagem:** C (Padrão C99/C11)
- **Compilador:** GCC
- **Ambiente de Desenvolvimento:** Linux / Bash / VS Code
- **Validação:** Ferramentas nativas `diff` e `md5sum`