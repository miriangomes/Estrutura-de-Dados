#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_SORTEIOS 5000

typedef struct No {
    int prioridade;
    struct No *proximo;
} No;

typedef struct {
    No *inicio;
} FilaSemHeap;

typedef struct {
    int dados[TOTAL_SORTEIOS + 1];
    int tamanho;
} FilaComHeap;

FilaSemHeap* criar_fila_sem_heap() {
    FilaSemHeap *f = (FilaSemHeap*)malloc(sizeof(FilaSemHeap));
    f->inicio = NULL;
    return f;
}

FilaComHeap* criar_fila_com_heap() {
    FilaComHeap *h = (FilaComHeap*)malloc(sizeof(FilaComHeap));
    h->tamanho = 0;
    return h;
}

int inserir_sem_heap(FilaSemHeap *f, int prioridade) {
    int comparacoes = 0;
    No *novo = (No*)malloc(sizeof(No));
    novo->prioridade = prioridade;
    novo->proximo = NULL;

    if (f->inicio == NULL) {
        comparacoes++;
        f->inicio = novo;
        return comparacoes;
    }
    
    comparacoes++;
    if (prioridade > f->inicio->prioridade) {
        novo->proximo = f->inicio;
        f->inicio = novo;
        return comparacoes;
    }
    
    No *atual = f->inicio;
    while (atual->proximo != NULL) {
        comparacoes++;
        if (atual->proximo->prioridade >= prioridade) {
            atual = atual->proximo;
        } else {
            break;
        }
    }
    
    novo->proximo = atual->proximo;
    atual->proximo = novo;
    return comparacoes;
}

void trocar(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int inserir_com_heap(FilaComHeap *h, int prioridade) {
    int comparacoes = 0;
    if (h->tamanho >= TOTAL_SORTEIOS) {
        return 0;
    }
    
    h->tamanho++;
    int i = h->tamanho;
    h->dados[i] = prioridade;
    
    while (i > 1) {
        int pai = i / 2;
        comparacoes++;
        if (h->dados[i] > h->dados[pai]) {
            trocar(&h->dados[i], &h->dados[pai]);
            i = pai;
        } else {
            break;
        }
    }
    return comparacoes;
}

void liberar_fila_sem_heap(FilaSemHeap *f) {
    No *atual = f->inicio;
    while (atual != NULL) {
        No *temp = atual;
        atual = atual->proximo;
        free(temp);
    }
    free(f);
}

int main() {
    srand((unsigned int)time(NULL));
    FilaSemHeap *fila_sem_heap = criar_fila_sem_heap();
    FilaComHeap *fila_com_heap = criar_fila_com_heap();
    
    FILE *arquivo = fopen("dados.csv", "w");
    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo dados.csv\n");
        return 1;
    }
    
    fprintf(arquivo, "Sorteio,NumeroSorteado,SemHeap,ComHeap\n");
    
    for (int i = 1; i <= TOTAL_SORTEIOS; i++) {
        int numero_sorteado = rand() % 100000;
        int comp_sem_heap = inserir_sem_heap(fila_sem_heap, numero_sorteado);
        int comp_com_heap = inserir_com_heap(fila_com_heap, numero_sorteado);
        
        fprintf(arquivo, "%d,%d,%d,%d\n", i, numero_sorteado, comp_sem_heap, comp_com_heap);
        
        if (i == TOTAL_SORTEIOS) {
            printf("Ultimo Numero sorteado: %d\n", numero_sorteado);
            printf("Numero de comparacoes na Fila sem Heap = %d\n", comp_sem_heap);
            printf("Numero de comparacoes na Fila com Heap = %d\n", comp_com_heap);
        }
    }
    
    fclose(arquivo);
    liberar_fila_sem_heap(fila_sem_heap);
    free(fila_com_heap);
    
    printf("Arquivo dados.csv gerado com sucesso!\n");
    return 0;
}