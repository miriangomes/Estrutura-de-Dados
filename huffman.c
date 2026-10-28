#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct No {
    void *byte;
    void *frequencia;
    struct No *esquerda, *direita, *proximo;
} No; // No é um atalho para não precisar escrever a estrutura toda sempre

No *criar_no(unsigned char caracter, long long frequencia, No *esquerda, No *direita) {
    No *novo_no = malloc(sizeof(No));
    novo_no->byte = malloc(sizeof(unsigned char));
    novo_no->frequencia = malloc(sizeof(long long));
    *(unsigned char *)novo_no->byte = caracter;
    *(long long *)novo_no->frequencia = frequencia;
    novo_no->esquerda = esquerda;
    novo_no->direita = direita;
    novo_no->proximo = NULL;
    return novo_no;
}

unsigned char get_byte(No *no) { return *(unsigned char *)no->byte; } // consulta e devolve
long long get_freq(No *no) { return *(long long *)no->frequencia; }
int eh_folha(No *no) { return no->esquerda == NULL && no->direita == NULL; }

int is_bit_i_set(unsigned char caracter, int posicao) { return caracter & (1 << posicao); } // checa e diz se é 0 ou 1 
unsigned char set_bit(unsigned char caracter, int posicao) { return caracter | (1 << posicao); } // força o lado a virar 1

void inserir(No **fila, No *novo) {
    if (*fila == NULL || get_freq(novo) <= get_freq(*fila)) {
        novo->proximo = *fila;
        *fila = novo;
        return;
    }
    No *atual = *fila;
    while (atual->proximo != NULL && get_freq(atual->proximo) < get_freq(novo))
        atual = atual->proximo;
    novo->proximo = atual->proximo; 
    atual->proximo = novo;
}

No *remover(No **fila) { //arranca do primeiro, move p o segundo, segundo vira o primeiro e depois desconecta o nó removiddo e devolve o nó retirado
    No *no = *fila;
    *fila = no->proximo;
    no->proximo = NULL;
    return no;
}

No *construir_arvore(long long freq[256]) {
    No *fila = NULL;
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0) inserir(&fila, criar_no(i, freq[i], NULL, NULL));

    if (fila == NULL) return NULL;
    if (fila->proximo == NULL) return criar_no('*', get_freq(fila), fila, NULL);

    while (fila->proximo != NULL) {
        No *primeiro = remover(&fila);
        No *segundo = remover(&fila);
        inserir(&fila, criar_no('*', get_freq(primeiro) + get_freq(segundo), primeiro, segundo));
    }
    return fila;
}

void liberar(No *no) {
    if (no == NULL) return;
    liberar(no->esquerda);
    liberar(no->direita);
    free(no->byte);
    free(no->frequencia);
    free(no);
}

void gerar_codigos(No *no, char codigos[256][256], char caminho[256], int profundidade) {
    if (no == NULL) return;
    if (eh_folha(no)) {
        caminho[profundidade] = '\0';
        strcpy(codigos[get_byte(no)], caminho);
        return;
    }
    caminho[profundidade] = '0';
    gerar_codigos(no->esquerda, codigos, caminho, profundidade + 1);
    caminho[profundidade] = '1';
    gerar_codigos(no->direita, codigos, caminho, profundidade + 1);
}

int tamanho_arvore(No *no) {
    if (no == NULL) return 0;
    if (eh_folha(no)) return (get_byte(no) == '*' || get_byte(no) == '\\') ? 2 : 1;
    return 1 + tamanho_arvore(no->esquerda) + tamanho_arvore(no->direita);
}

void escrever_arvore(No *no, FILE *arquivo) {
    if (no == NULL) return;
    if (eh_folha(no)) {
        if (get_byte(no) == '*' || get_byte(no) == '\\') fputc('\\', arquivo);
        fputc(get_byte(no), arquivo);
        return;
    }
    fputc('*', arquivo);
    escrever_arvore(no->esquerda, arquivo);
    escrever_arvore(no->direita, arquivo);
}

No *ler_arvore(FILE *arquivo, int *restantes) {
    if (*restantes <= 0) return NULL;
    int caracter_lido = fgetc(arquivo); // retira e armazena
    (*restantes)--;

    if (caracter_lido == '*') {
        No *pai = criar_no('*', 0, NULL, NULL);
        pai->esquerda = ler_arvore(arquivo, restantes);
        pai->direita = ler_arvore(arquivo, restantes);
        return pai;
    }
    if (caracter_lido == '\\') {
        caracter_lido = fgetc(arquivo);
        (*restantes)--;
    }
    return criar_no(caracter_lido, 0, NULL, NULL);
}

void compactar(const char *nome_entrada, const char *nome_saida) {
    FILE *entrada = fopen(nome_entrada, "rb"); //read binary e fopen vem de file open
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada!\n");
        return;
    }

    long long freq[256] = {0};
    int caracter_lido;
    while ((caracter_lido = fgetc(entrada)) != EOF) freq[caracter_lido]++; //end of file

    No *raiz = construir_arvore(freq);
    char codigos[256][256] = {{0}}, caminho[256];
    gerar_codigos(raiz, codigos, caminho, 0);

    long long total_bits = 0;
    for (int i = 0; i < 256; i++) total_bits += freq[i] * strlen(codigos[i]); //qts vezes o caracter apareceu no arquivo original - multiplicação
    int lixo = (8 - total_bits % 8) % 8; //repetição do % trata casos perfeitos
    int tam_arvore = tamanho_arvore(raiz);

    FILE *saida = fopen(nome_saida, "wb");
    fputc((lixo << 5) | (tam_arvore >> 8), saida); // bits em cima e bits em baixo
    fputc(tam_arvore & 0xFF, saida); //8 bits ligaado - 11111111
    escrever_arvore(raiz, saida);

    rewind(entrada);
    unsigned char atual = 0; //acumulador
    int pos = 7;
    while ((caracter_lido = fgetc(entrada)) != EOF) {
        for (char *p = codigos[caracter_lido]; *p != '\0'; p++) {
            if (*p == '1') atual = set_bit(atual, pos);
            pos--;
            if (pos < 0) {
                fputc(atual, saida);
                atual = 0;
                pos = 7;
            }
        }
    }

    if (pos != 7) fputc(atual, saida); // sobraram bits pendentes

    liberar(raiz);
    fclose(entrada);
    fclose(saida);
    printf("Compactacao concluida com sucesso!\n");
}

void descompactar(const char *nome_huff, const char *nome_saida) {
    FILE *entrada = fopen(nome_huff, "rb");
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo compactado!\n");
        return;
    }

    int byte1 = fgetc(entrada), byte2 = fgetc(entrada); //bits em cima e bits em baixo
    if (byte1 == EOF || byte2 == EOF) {
        printf("Arquivo .huff invalido!\n");
        fclose(entrada);
        return;
    }

    int lixo = byte1 >> 5; //recupera os 3 bits
    int tam_arvore = ((byte1 & 0x1F) << 8) | byte2; //encaixa os 8 bits baixos restantes

    No *raiz = ler_arvore(entrada, &tam_arvore); //reconstruir
    FILE *saida = fopen(nome_saida, "wb");

    No *atual = raiz;
    int byte_lido = fgetc(entrada);

    while (byte_lido != EOF) {
        int prox_byte = fgetc(entrada);

        int fim;
        if (prox_byte == EOF) {
            fim = lixo;
        } else {
            fim = 0;
        }

        for (int pos = 7; pos >= fim; pos--) {
            if (is_bit_i_set(byte_lido, pos)) {
                atual = atual->direita;
            } else {
                atual = atual->esquerda;
            }

            if (eh_folha(atual)) {
                fputc(get_byte(atual), saida);
                atual = raiz;
            }
        }

        byte_lido = prox_byte;
    }

    liberar(raiz);
    fclose(entrada);
    fclose(saida);
    printf("Descompactacao concluida com sucesso!\n");
}

int main(int argc, char *argv[]) {
    // Execução direta por linha de comando (CLI)
    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) {
            compactar(argv[2], argv[3]);
        } else if (strcmp(argv[1], "-d") == 0) {
            descompactar(argv[2], argv[3]);
        }
        return 0;
    }

    // Menu interativo (caso não passe os 4 argumentos)
    int opcao;
    char origem[256];
    char destino[256];

    printf("1 - Compactar arquivo\n2 - Descompactar arquivo\nEscolha: ");
    if (scanf("%d", &opcao) != 1) {
        return 0;
    }

    printf("Arquivo de entrada: ");
    if (scanf("%255s", origem) != 1) {
        return 0;
    }

    printf("Arquivo de saida: ");
    if (scanf("%255s", destino) != 1) {
        return 0;
    }

    if (opcao == 1) {
        compactar(origem, destino);
    } else if (opcao == 2) {
        descompactar(origem, destino);
    }

    return 0;
}