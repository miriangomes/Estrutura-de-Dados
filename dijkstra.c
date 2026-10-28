#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define INF INT_MAX

typedef struct NoAdj {
    int destino;
    int peso;
    struct NoAdj* proximo;
} NoAdj;

typedef struct {
    int num_vertices;
    int num_arestas;
    NoAdj** lista_adj;
} Grafo;

typedef struct {
    void** dados;
    int tamanho;
    int capacidade;
    int (*comparar)(const void*, const void*);
} MinHeap;

typedef struct {
    int vertice;
    int distancia;
} NoDijkstra;

Grafo* criar_grafo(int vertices, int arestas) {
    Grafo* g = (Grafo*)malloc(sizeof(Grafo));
    g->num_vertices = vertices;
    g->num_arestas = arestas;
    g->lista_adj = (NoAdj**)malloc(vertices * sizeof(NoAdj*));
    for (int i = 0; i < vertices; i++) {
        g->lista_adj[i] = NULL;
    }
    return g;
}

void adicionar_aresta(Grafo* g, int u, int v, int peso) {
    NoAdj* novo = (NoAdj*)malloc(sizeof(NoAdj));
    novo->destino = v;
    novo->peso = peso;
    novo->proximo = g->lista_adj[u];
    g->lista_adj[u] = novo;

    NoAdj* novo_inv = (NoAdj*)malloc(sizeof(NoAdj));
    novo_inv->destino = u;
    novo_inv->peso = peso;
    novo_inv->proximo = g->lista_adj[v];
    g->lista_adj[v] = novo_inv;
}

MinHeap* criar_heap(int capacidade, int (*comparar)(const void*, const void*)) {
    MinHeap* h = (MinHeap*)malloc(sizeof(MinHeap));
    h->dados = (void**)malloc(capacidade * sizeof(void*));
    h->tamanho = 0;
    h->capacidade = capacidade;
    h->comparar = comparar;
    return h;
}

void trocar(void** a, void** b) {
    void* temp = *a;
    *a = *b;
    *b = temp;
}

void inserir_heap(MinHeap* h, void* item) {
    if (h->tamanho == h->capacidade) return;
    int i = h->tamanho++;
    h->dados[i] = item;

    while (i != 0 && h->comparar(h->dados[i], h->dados[(i - 1) / 2]) < 0) {
        trocar(&h->dados[i], &h->dados[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void* extrair_min(MinHeap* h) {
    if (h->tamanho <= 0) return NULL;
    if (h->tamanho == 1) return h->dados[--h->tamanho];

    void* raiz = h->dados[0];
    h->dados[0] = h->dados[--h->tamanho];

    int i = 0;
    while (1) {
        int esq = 2 * i + 1;
        int dir = 2 * i + 2;
        int menor = i;

        if (esq < h->tamanho && h->comparar(h->dados[esq], h->dados[menor]) < 0)
            menor = esq;
        if (dir < h->tamanho && h->comparar(h->dados[dir], h->dados[menor]) < 0)
            menor = dir;

        if (menor != i) {
            trocar(&h->dados[i], &h->dados[menor]);
            i = menor;
        } else {
            break;
        }
    }
    return raiz;
}

int comparar_dijkstra(const void* a, const void* b) {
    NoDijkstra* n1 = (NoDijkstra*)a;
    NoDijkstra* n2 = (NoDijkstra*)b;
    return n1->distancia - n2->distancia;
}

void dijkstra(Grafo* g, int origem) {
    int V = g->num_vertices;
    int* dist = (int*)malloc(V * sizeof(int));
    int* predecessor = (int*)malloc(V * sizeof(int));
    int* visitado = (int*)calloc(V, sizeof(int));

    for (int i = 0; i < V; i++) {
        dist[i] = INF;
        predecessor[i] = -1;
    }
    dist[origem] = 0;

    MinHeap* heap = criar_heap(g->num_arestas * 4 + V, comparar_dijkstra);

    NoDijkstra* inicial = (NoDijkstra*)malloc(sizeof(NoDijkstra));
    inicial->vertice = origem;
    inicial->distancia = 0;
    inserir_heap(heap, (void*)inicial);

    while (heap->tamanho > 0) {
        NoDijkstra* atual = (NoDijkstra*)extrair_min(heap);
        int u = atual->vertice;
        free(atual);

        if (visitado[u]) continue;
        visitado[u] = 1;

        NoAdj* temp = g->lista_adj[u];
        while (temp != NULL) {
            int v = temp->destino;
            int peso = temp->peso;

            if (!visitado[v] && dist[u] != INF && dist[u] + peso < dist[v]) {
                dist[v] = dist[u] + peso;
                predecessor[v] = u;

                NoDijkstra* novo = (NoDijkstra*)malloc(sizeof(NoDijkstra));
                novo->vertice = v;
                novo->distancia = dist[v];
                inserir_heap(heap, (void*)novo);
            }
            temp = temp->proximo;
        }
    }

    printf("Caminhos Minimos a partir do vertice %d (Dijkstra):\n", origem);
    for (int i = 0; i < V; i++) {
        printf("Vertice %d -> Distancia: %d (Predecessor: %d)\n", i, dist[i], predecessor[i]);
    }

    free(dist);
    free(predecessor);
    free(visitado);
    free(heap->dados);
    free(heap);
}

void liberar_grafo(Grafo* g) {
    for (int i = 0; i < g->num_vertices; i++) {
        NoAdj* atual = g->lista_adj[i];
        while (atual != NULL) {
            NoAdj* temp = atual;
            atual = atual->proximo;
            free(temp);
        }
    }
    free(g->lista_adj);
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

    dijkstra(g, 0);

    liberar_grafo(g);
    return 0;
}