#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct No {
    void *byte;
    void *freq;
    struct No *esq, *dir, *prox;
} No;

No *criar_no(unsigned char b, long long f, No *esq, No *dir) {
    No *no = malloc(sizeof(No));
    no->byte = malloc(sizeof(unsigned char));
    no->freq = malloc(sizeof(long long));
    *(unsigned char *)no->byte = b;
    *(long long *)no->freq = f;
    no->esq = esq;
    no->dir = dir;
    no->prox = NULL;
    return no;
}

unsigned char get_byte(No *no) { return *(unsigned char *)no->byte; }
long long get_freq(No *no) { return *(long long *)no->freq; }
int eh_folha(No *no) { return no->esq == NULL && no->dir == NULL; }

int is_bit_i_set(unsigned char c, int i) { return c & (1 << i); }
unsigned char set_bit(unsigned char c, int i) { return c | (1 << i); }

void inserir(No **fila, No *novo) {
    if (*fila == NULL || get_freq(novo) <= get_freq(*fila)) {
        novo->prox = *fila;
        *fila = novo;
        return;
    }
    No *atual = *fila;
    while (atual->prox != NULL && get_freq(atual->prox) < get_freq(novo))
        atual = atual->prox;
    novo->prox = atual->prox;
    atual->prox = novo;
}

No *remover(No **fila) {
    No *no = *fila;
    *fila = no->prox;
    no->prox = NULL;
    return no;
}

No *construir_arvore(long long freq[256]) {
    No *fila = NULL;
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0) inserir(&fila, criar_no(i, freq[i], NULL, NULL));

    if (fila == NULL) return NULL;
    if (fila->prox == NULL) return criar_no('*', get_freq(fila), fila, NULL);

    while (fila->prox != NULL) {
        No *a = remover(&fila);
        No *b = remover(&fila);
        inserir(&fila, criar_no('*', get_freq(a) + get_freq(b), a, b));
    }
    return fila;
}

void liberar(No *no) {
    if (no == NULL) return;
    liberar(no->esq);
    liberar(no->dir);
    free(no->byte);
    free(no->freq);
    free(no);
}

void gerar_codigos(No *no, char codigos[256][256], char caminho[256], int prof) {
    if (no == NULL) return;
    if (eh_folha(no)) {
        caminho[prof] = '\0';
        strcpy(codigos[get_byte(no)], caminho);
        return;
    }
    caminho[prof] = '0';
    gerar_codigos(no->esq, codigos, caminho, prof + 1);
    caminho[prof] = '1';
    gerar_codigos(no->dir, codigos, caminho, prof + 1);
}

int tamanho_arvore(No *no) {
    if (no == NULL) return 0;
    if (eh_folha(no)) return (get_byte(no) == '*' || get_byte(no) == '\\') ? 2 : 1;
    return 1 + tamanho_arvore(no->esq) + tamanho_arvore(no->dir);
}

void escrever_arvore(No *no, FILE *f) {
    if (no == NULL) return;
    if (eh_folha(no)) {
        if (get_byte(no) == '*' || get_byte(no) == '\\') fputc('\\', f);
        fputc(get_byte(no), f);
        return;
    }
    fputc('*', f);
    escrever_arvore(no->esq, f);
    escrever_arvore(no->dir, f);
}

No *ler_arvore(FILE *f, int *restantes) {
    if (*restantes <= 0) return NULL;
    int c = fgetc(f);
    (*restantes)--;

    if (c == '*') {
        No *pai = criar_no('*', 0, NULL, NULL);
        pai->esq = ler_arvore(f, restantes);
        pai->dir = ler_arvore(f, restantes);
        return pai;
    }
    if (c == '\\') {
        c = fgetc(f);
        (*restantes)--;
    }
    return criar_no(c, 0, NULL, NULL);
}

void compactar(const char *nome_entrada, const char *nome_saida) {
    FILE *in = fopen(nome_entrada, "rb");
    if (in == NULL) {
        printf("Erro ao abrir o arquivo de entrada!\n");
        return;
    }

    long long freq[256] = {0};
    int c;
    while ((c = fgetc(in)) != EOF) freq[c]++;

    No *raiz = construir_arvore(freq);
    char codigos[256][256] = {{0}}, caminho[256];
    gerar_codigos(raiz, codigos, caminho, 0);

    long long total_bits = 0;
    for (int i = 0; i < 256; i++) total_bits += freq[i] * strlen(codigos[i]);
    int lixo = (8 - total_bits % 8) % 8;
    int tam = tamanho_arvore(raiz);

    FILE *out = fopen(nome_saida, "wb");
    fputc((lixo << 5) | (tam >> 8), out);
    fputc(tam & 0xFF, out);
    escrever_arvore(raiz, out);

    rewind(in);
    unsigned char atual = 0;
    int pos = 7;
    while ((c = fgetc(in)) != EOF) {
        for (char *p = codigos[c]; *p != '\0'; p++) {
            if (*p == '1') atual = set_bit(atual, pos);
            pos--;
            if (pos < 0) {
                fputc(atual, out);
                atual = 0;
                pos = 7;
            }
        }
    }
    if (pos != 7) fputc(atual, out);

    liberar(raiz);
    fclose(in);
    fclose(out);
    printf("Compactacao concluida com sucesso!\n");
}

void descompactar(const char *nome_huff, const char *nome_saida) {
    FILE *in = fopen(nome_huff, "rb");
    if (in == NULL) {
        printf("Erro ao abrir o arquivo compactado!\n");
        return;
    }

    int b1 = fgetc(in), b2 = fgetc(in);
    if (b1 == EOF || b2 == EOF) {
        printf("Arquivo .huff invalido!\n");
        fclose(in);
        return;
    }
    int lixo = b1 >> 5;
    int tam = ((b1 & 0x1F) << 8) | b2;

    No *raiz = ler_arvore(in, &tam);
    FILE *out = fopen(nome_saida, "wb");

    No *atual = raiz;
    int byte = fgetc(in);
    while (byte != EOF) {
        int prox = fgetc(in);
        int fim = (prox == EOF) ? lixo : 0;

        for (int pos = 7; pos >= fim; pos--) {
            atual = is_bit_i_set(byte, pos) ? atual->dir : atual->esq;
            if (eh_folha(atual)) {
                fputc(get_byte(atual), out);
                atual = raiz;
            }
        }
        byte = prox;
    }

    liberar(raiz);
    fclose(in);
    fclose(out);
    printf("Descompactacao concluida com sucesso!\n");
}

int main(int argc, char *argv[]) {
    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) compactar(argv[2], argv[3]);
        else if (strcmp(argv[1], "-d") == 0) descompactar(argv[2], argv[3]);
        return 0;
    }

    int opcao;
    char origem[256], destino[256];

    printf("1 - Compactar arquivo\n2 - Descompactar arquivo\nEscolha: ");
    if (scanf("%d", &opcao) != 1) return 0;
    printf("Arquivo de entrada: ");
    if (scanf("%255s", origem) != 1) return 0;
    printf("Arquivo de saida: ");
    if (scanf("%255s", destino) != 1) return 0;

    if (opcao == 1) compactar(origem, destino);
    else if (opcao == 2) descompactar(origem, destino);
    return 0;
}