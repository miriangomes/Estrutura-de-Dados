#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int origem;
    int destino;
    int peso;
} Aresta;

typedef struct {
    int num_vertices;
    int num_arestas;
    int arestas_inseridas;
    Aresta* arestas;
} Grafo;

typedef struct {
    int* pai;
    int* rank;
} UnionFind;

Grafo* criar_grafo(int vertices, int arestas) {
    Grafo* g = (Grafo*)malloc(sizeof(Grafo));
    g->num_vertices = vertices;
    g->num_arestas = arestas;
    g->arestas_inseridas = 0;
    g->arestas = (Aresta*)malloc(arestas * sizeof(Aresta));
    return g;
}

void adicionar_aresta(Grafo* g, int u, int v, int peso) {
    if (g->arestas_inseridas < g->num_arestas) {
        g->arestas[g->arestas_inseridas].origem = u;
        g->arestas[g->arestas_inseridas].destino = v;
        g->arestas[g->arestas_inseridas].peso = peso;
        g->arestas_inseridas++;
    }
}

UnionFind* criar_union_find(int n) {
    UnionFind* uf = (UnionFind*)malloc(sizeof(UnionFind));
    uf->pai = (int*)malloc(n * sizeof(int));
    uf->rank = (int*)calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) {
        uf->pai[i] = i;
    }
    return uf;
}

int encontrar(UnionFind* uf, int i) {
    if (uf->pai[i] != i) {
        uf->pai[i] = encontrar(uf, uf->pai[i]);
    }
    return uf->pai[i];
}

void unir(UnionFind* uf, int x, int y) {
    int raiz_x = encontrar(uf, x);
    int raiz_y = encontrar(uf, y);

    if (raiz_x != raiz_y) {
        if (uf->rank[raiz_x] < uf->rank[raiz_y]) {
            uf->pai[raiz_x] = raiz_y;
        } else if (uf->rank[raiz_x] > uf->rank[raiz_y]) {
            uf->pai[raiz_y] = raiz_x;
        } else {
            uf->pai[raiz_y] = raiz_x;
            uf->rank[raiz_x]++;
        }
    }
}

void intercala(Aresta* v, int inicio, int meio, int fim) {
    int n1 = meio - inicio + 1;
    int n2 = fim - meio;
    Aresta* esq = (Aresta*)malloc(n1 * sizeof(Aresta));
    Aresta* dir = (Aresta*)malloc(n2 * sizeof(Aresta));

    for (int i = 0; i < n1; i++) esq[i] = v[inicio + i];
    for (int j = 0; j < n2; j++) dir[j] = v[meio + 1 + j];

    int i = 0, j = 0, k = inicio;
    while (i < n1 && j < n2) {
        if (esq[i].peso <= dir[j].peso) {
            v[k++] = esq[i++];
        } else {
            v[k++] = dir[j++];
        }
    }
    while (i < n1) v[k++] = esq[i++];
    while (j < n2) v[k++] = dir[j++];

    free(esq);
    free(dir);
}

void merge_sort(Aresta* v, int inicio, int fim) {
    if (inicio < fim) {
        int meio = inicio + (fim - inicio) / 2;
        merge_sort(v, inicio, meio);
        merge_sort(v, meio + 1, fim);
        intercala(v, inicio, meio, fim);
    }
}

void kruskal(Grafo* g) {
    int V = g->num_vertices;
    Aresta* resultado = (Aresta*)malloc((V - 1) * sizeof(Aresta));
    int e = 0;
    int i = 0;

    merge_sort(g->arestas, 0, g->arestas_inseridas - 1);

    UnionFind* uf = criar_union_find(V);

    while (e < V - 1 && i < g->arestas_inseridas) {
        Aresta proxima = g->arestas[i++];

        int x = encontrar(uf, proxima.origem);
        int y = encontrar(uf, proxima.destino);

        if (x != y) {
            resultado[e++] = proxima;
            unir(uf, x, y);
        }
    }

    printf("Arvore Geradora Minima (Kruskal):\n");
    int custo_total = 0;
    for (i = 0; i < e; i++) {
        printf("Aresta (%d -- %d) -> Peso: %d\n", resultado[i].origem, resultado[i].destino, resultado[i].peso);
        custo_total += resultado[i].peso;
    }
    printf("Custo Total: %d\n", custo_total);

    free(uf->pai);
    free(uf->rank);
    free(uf);
    free(resultado);
}

void liberar_grafo(Grafo* g) {
    free(g->arestas);
    free(g);
}

int main() {
    Grafo* g = criar_grafo(6, 9);

    adicionar_aresta(g, 0, 1, 4);
    adicionar_aresta(g, 0, 2, 4);
    adicionar_aresta(g, 1, 2, 2);
    adicionar_aresta(g, 1, 3, 5);
    adicionar_aresta(g, 2, 3, 5);
    adicionar_aresta(g, 2, 4, 11);
    adicionar_aresta(g, 3, 4, 2);
    adicionar_aresta(g, 3, 5, 6);
    adicionar_aresta(g, 4, 5, 3);

    kruskal(g);

    liberar_grafo(g);
    return 0;
}