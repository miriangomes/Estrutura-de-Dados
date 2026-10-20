/**
 * @file huffman.c
 * @brief Compactador/descompactador de arquivos (Codificacao de Huffman).
 *
 * Funciona com qualquer tipo de arquivo (opera sobre bytes).
 * Formato do .huff: [3 bits de lixo][13 bits tamanho da arvore][arvore em pre-ordem][dados]
 *
 * Compilar: gcc -Wall -Wextra -o huffman huffman.c
 * Usar:     ./huffman -c original saida.huff
 *           ./huffman -d saida.huff recuperado
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_ALFABETO 256
#define TAM_MAX_CODIGO 256

/**
 * @brief Confere se um malloc deu certo. Se nao deu, encerra o programa.
 * @param ptr Endereco devolvido pelo malloc.
 * @return O mesmo endereco (para podermos usar em uma linha so).
 */
static void *checar_alocacao(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Erro: sem memoria!\n");
        exit(1);
    }
    return ptr;
}

/**
 * @brief No da arvore de Huffman.
 *
 * Os DADOS (byte e frequencia) ficam em void*.
 * Os filhos sao ponteiros normais para outros nos.
 */
typedef struct NoArvore {
    void *byte;
    void *frequencia;
    struct NoArvore *esq;
    struct NoArvore *dir;
} NoArvore;

/**
 * @brief Cria um no da arvore.
 * @param byte  Byte guardado no no (nos internos usam '*', nao importa).
 * @param freq  Frequencia do no.
 * @param esq   Filho esquerdo (NULL se for folha).
 * @param dir   Filho direito  (NULL se for folha).
 * @return Endereco do novo no.
 */
NoArvore *arvore_criar_no(unsigned char byte, long long freq,
                          NoArvore *esq, NoArvore *dir) {

    NoArvore *no = checar_alocacao(malloc(sizeof(NoArvore)));

    no->byte       = checar_alocacao(malloc(sizeof(unsigned char)));
    no->frequencia = checar_alocacao(malloc(sizeof(long long)));

    *(unsigned char *)no->byte       = byte;
    *(long long *)no->frequencia     = freq;

    no->esq = esq;
    no->dir = dir;
    return no;
}

/** @brief Le o byte guardado em um no. */
unsigned char arvore_byte(const NoArvore *no) {
    return *(unsigned char *)no->byte;
}

/** @brief Le a frequencia guardada em um no. */
long long arvore_frequencia(const NoArvore *no) {
    return *(long long *)no->frequencia;
}

/**
 * @brief Diz se o no e folha (nao tem nenhum filho).
 * @note  E assim (e nao pelo byte '*') que diferenciamos uma folha que
 *        guarda o caractere '*' de um no interno.
 */
int arvore_eh_folha(const NoArvore *no) {
    return (no->esq == NULL && no->dir == NULL);
}

/**
 * @brief Libera a arvore inteira da memoria (recursivo, pos-ordem).
 * @param raiz Raiz da arvore (pode ser NULL).
 */
void arvore_liberar(NoArvore *raiz) {
    if (raiz == NULL) return;
    arvore_liberar(raiz->esq);
    arvore_liberar(raiz->dir);
    free(raiz->byte);
    free(raiz->frequencia);
    free(raiz);
}

/**
 * @brief Tipo "funcao de comparacao".
 *
 * A fila nao sabe o que tem dentro dos void*. Entao quem usa a fila
 * ENTREGA uma funcao que diz qual item vem primeiro:
 *   retorna < 0 se a e "menor" que b
 *   retorna   0 se sao iguais
 *   retorna > 0 se a e "maior" que b
 */
typedef int (*FuncaoComparar)(const void *a, const void *b);

/** @brief No da lista encadeada da fila. */
typedef struct NoFila {
    void *item;
    struct NoFila *proximo;
} NoFila;

/** @brief Fila de prioridade (lista encadeada ORDENADA, menor primeiro). */
typedef struct {
    NoFila *inicio;
    int tamanho;
} Fila;

/** @brief Cria uma fila vazia. */
Fila *fila_criar(void) {
    Fila *fila = checar_alocacao(malloc(sizeof(Fila)));
    fila->inicio = NULL;
    fila->tamanho = 0;
    return fila;
}

/**
 * @brief Insere um item mantendo a fila em ordem crescente.
 * @param fila     A fila.
 * @param item     Endereco do item a guardar.
 * @param comparar Funcao que compara dois itens.
 * @note Em caso de empate o novo item entra ANTES dos iguais (igual ao
 *       exemplo da Aula 20).
 */
void fila_inserir(Fila *fila, void *item, FuncaoComparar comparar) {
    NoFila *novo = checar_alocacao(malloc(sizeof(NoFila)));
    novo->item = item;
    novo->proximo = NULL;

    if (fila->inicio == NULL || comparar(item, fila->inicio->item) <= 0) {
        novo->proximo = fila->inicio;
        fila->inicio = novo;
    } else {

        NoFila *atual = fila->inicio;
        while (atual->proximo != NULL &&
               comparar(atual->proximo->item, item) < 0) {
            atual = atual->proximo;
        }

        novo->proximo = atual->proximo;
        atual->proximo = novo;
    }
    fila->tamanho++;
}

/**
 * @brief Remove e devolve o item do inicio (o menor).
 * @return Endereco do item, ou NULL se a fila estiver vazia.
 */
void *fila_remover(Fila *fila) {
    if (fila->inicio == NULL) return NULL;

    NoFila *removido = fila->inicio;
    void *item = removido->item;
    fila->inicio = removido->proximo;
    free(removido);
    fila->tamanho--;
    return item;
}

/** @brief Libera a fila (so chame com a fila vazia). */
void fila_liberar(Fila *fila) {
    free(fila);
}

/**
 * @brief Verifica se o bit i de um byte vale 1.
 * @param c Byte.
 * @param i Posicao do bit (0 = mais a direita, 7 = mais a esquerda).
 * @return Diferente de 0 se o bit for 1; 0 se for 0.
 *
 * Exemplo: c = 11000111, i = 4
 *   mask = 1 << 4 = 00010000
 *   mask & c      = 00000000  -> bit 4 NAO esta ligado
 */
int is_bit_i_set(unsigned char c, int i) {
    unsigned char mask = 1 << i;
    return mask & c;
}

/**
 * @brief Liga (coloca 1) o bit i de um byte.
 * @return O byte com o bit i ligado.
 *
 * Exemplo: c = 11000111, i = 4
 *   mask = 00010000
 *   mask | c      = 11010111  -> bit 4 agora esta ligado
 */
unsigned char set_bit(unsigned char c, int i) {
    unsigned char mask = 1 << i;
    return mask | c;
}

/**
 * @brief Conta quantas vezes cada byte (0..255) aparece no arquivo.
 * @param arquivo Arquivo aberto em modo binario ("rb").
 * @param freq    Vetor de 256 posicoes; freq[65] = quantos 'A' (65 = 'A').
 */
void contar_frequencias(FILE *arquivo, long long freq[TAM_ALFABETO]) {
    int c;
    while ((c = fgetc(arquivo)) != EOF) {
        freq[c]++;
    }
}

/**
 * @brief Compara dois nos de arvore pela frequencia (usada pela fila).
 */
int comparar_nos(const void *a, const void *b) {
    long long fa = arvore_frequencia((const NoArvore *)a);
    long long fb = arvore_frequencia((const NoArvore *)b);
    return (fa > fb) - (fa < fb);
}

/**
 * @brief Monta a arvore de Huffman a partir das frequencias.
 * @return Raiz da arvore (NULL se o arquivo era vazio).
 */
NoArvore *construir_arvore(long long freq[TAM_ALFABETO]) {
    Fila *fila = fila_criar();

    for (int i = 0; i < TAM_ALFABETO; i++) {
        if (freq[i] > 0) {
            NoArvore *folha = arvore_criar_no((unsigned char)i, freq[i], NULL, NULL);
            fila_inserir(fila, folha, comparar_nos);
        }
    }

    if (fila->tamanho == 0) {
        fila_liberar(fila);
        return NULL;
    }

    if (fila->tamanho == 1) {
        NoArvore *unica = fila_remover(fila);
        fila_liberar(fila);
        return arvore_criar_no('*', arvore_frequencia(unica), unica, NULL);
    }

    while (fila->tamanho > 1) {
        NoArvore *esq = fila_remover(fila);
        NoArvore *dir = fila_remover(fila);
        long long soma = arvore_frequencia(esq) + arvore_frequencia(dir);
        NoArvore *pai = arvore_criar_no('*', soma, esq, dir);
        fila_inserir(fila, pai, comparar_nos);
    }

    NoArvore *raiz = fila_remover(fila);
    fila_liberar(fila);
    return raiz;
}

/**
 * @brief Percorre a arvore e preenche a tabela de codigos.
 *
 * Descer pela esquerda = escreve '0'. Descer pela direita = escreve '1'.
 * Ao chegar numa folha, o caminho percorrido e o codigo daquele byte.
 *
 * @param no          No atual da recursao.
 * @param codigos     codigos[b] recebe o codigo (texto "0101") do byte b.
 * @param caminho     Texto temporario com o caminho ate aqui.
 * @param profundidade Quantos passos ja descemos.
 */
void gerar_codigos(const NoArvore *no, char codigos[TAM_ALFABETO][TAM_MAX_CODIGO],
                   char caminho[TAM_MAX_CODIGO], int profundidade) {
    if (no == NULL) return;

    if (arvore_eh_folha(no)) {
        caminho[profundidade] = '\0';
        strcpy(codigos[arvore_byte(no)], caminho);
        return;
    }

    caminho[profundidade] = '0';
    gerar_codigos(no->esq, codigos, caminho, profundidade + 1);

    caminho[profundidade] = '1';
    gerar_codigos(no->dir, codigos, caminho, profundidade + 1);
}

/**
 * @brief Calcula quantos bits de LIXO sobram no ultimo byte (0 a 7).
 */
int calcular_lixo(long long freq[TAM_ALFABETO],
                  char codigos[TAM_ALFABETO][TAM_MAX_CODIGO]) {
    long long total_bits = 0;
    for (int i = 0; i < TAM_ALFABETO; i++) {

        total_bits += freq[i] * (long long)strlen(codigos[i]);
    }
    int resto = total_bits % 8;
    return (resto == 0) ? 0 : 8 - resto;
}

/**
 * @brief Calcula quantos BYTES a arvore ocupa no arquivo (pre-ordem).
 *
 * No interno = 1 byte ('*'). Folha = 1 byte, ou 2 se for '*' ou '\'
 * (porque precisa do '\' de escape na frente).
 */
int calcular_tamanho_arvore(const NoArvore *no) {
    if (no == NULL) return 0;
    if (arvore_eh_folha(no)) {
        unsigned char c = arvore_byte(no);
        return (c == '*' || c == '\\') ? 2 : 1;
    }
    return 1 + calcular_tamanho_arvore(no->esq) + calcular_tamanho_arvore(no->dir);
}

/**
 * @brief Escreve a arvore no arquivo em PRE-ORDEM (raiz, esquerda, direita).
 *
 * Exemplo do slide: **CB***FEDA
 */
void escrever_arvore(const NoArvore *no, FILE *saida) {
    if (no == NULL) return;

    if (arvore_eh_folha(no)) {
        unsigned char c = arvore_byte(no);
        if (c == '*' || c == '\\') {
            fputc('\\', saida);
        }
        fputc(c, saida);
        return;
    }

    fputc('*', saida);
    escrever_arvore(no->esq, saida);
    escrever_arvore(no->dir, saida);
}

/**
 * @brief Escreve os 2 primeiros bytes (lixo + tamanho) e depois a arvore.
 *
 * 16 bits:  [lll][ttttt ttttttttt]
 *            3 bits de lixo + 13 bits de tamanho da arvore
 */
void escrever_cabecalho(FILE *saida, int lixo, int tam_arvore, const NoArvore *raiz) {

    unsigned char byte1 = (lixo << 5) | (tam_arvore >> 8);

    unsigned char byte2 = tam_arvore & 0xFF;

    fputc(byte1, saida);
    fputc(byte2, saida);
    escrever_arvore(raiz, saida);
}

/**
 * @brief Compacta um arquivo qualquer.
 * @param nome_entrada Arquivo original.
 * @param nome_saida   Arquivo .huff a ser criado.
 * @return 0 se deu certo, 1 se deu erro.
 */
int compactar_arquivo(const char *nome_entrada, const char *nome_saida) {
    FILE *entrada = fopen(nome_entrada, "rb");
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada!\n");
        return 1;
    }

    long long freq[TAM_ALFABETO] = {0};
    contar_frequencias(entrada, freq);

    NoArvore *raiz = construir_arvore(freq);

    char codigos[TAM_ALFABETO][TAM_MAX_CODIGO] = {{0}};
    char caminho[TAM_MAX_CODIGO];
    gerar_codigos(raiz, codigos, caminho, 0);

    int lixo = calcular_lixo(freq, codigos);
    int tam_arvore = calcular_tamanho_arvore(raiz);

    FILE *saida = fopen(nome_saida, "wb");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida!\n");
        arvore_liberar(raiz);
        fclose(entrada);
        return 1;
    }
    escrever_cabecalho(saida, lixo, tam_arvore, raiz);

    rewind(entrada);

    unsigned char byte_atual = 0;
    int pos = 7;
    int c;

    while ((c = fgetc(entrada)) != EOF) {
        const char *codigo = codigos[c];

        for (int j = 0; codigo[j] != '\0'; j++) {
            if (codigo[j] == '1') {
                byte_atual = set_bit(byte_atual, pos);
            }
            pos--;

            if (pos < 0) {
                fputc(byte_atual, saida);
                byte_atual = 0;
                pos = 7;
            }
        }
    }

    if (pos != 7) {
        fputc(byte_atual, saida);
    }

    arvore_liberar(raiz);
    fclose(entrada);
    fclose(saida);
    printf("Compactacao concluida com sucesso!\n");
    return 0;
}

/**
 * @brief Reconstroi a arvore lendo o arquivo em pre-ordem.
 * @param huff      Arquivo .huff, posicionado no inicio da arvore.
 * @param restantes Quantos bytes da arvore ainda faltam ler (passado por
 *                  ENDERECO, para que todas as chamadas recursivas
 *                  enxerguem e atualizem o mesmo contador).
 * @return Raiz (sub)arvore lida.
 */
NoArvore *reconstruir_arvore(FILE *huff, int *restantes) {
    if (*restantes <= 0) return NULL;

    int c = fgetc(huff);
    if (c == EOF) return NULL;
    (*restantes)--;

    if (c == '*') {
        NoArvore *pai = arvore_criar_no('*', 0, NULL, NULL);
        pai->esq = reconstruir_arvore(huff, restantes);
        pai->dir = reconstruir_arvore(huff, restantes);
        return pai;
    }

    if (c == '\\') {
        c = fgetc(huff);
        (*restantes)--;
    }

    return arvore_criar_no((unsigned char)c, 0, NULL, NULL);
}

/**
 * @brief Descompacta um arquivo .huff.
 * @param nome_huff  Arquivo compactado.
 * @param nome_saida Arquivo recuperado.
 * @return 0 se deu certo, 1 se deu erro.
 */
int descompactar_arquivo(const char *nome_huff, const char *nome_saida) {
    FILE *huff = fopen(nome_huff, "rb");
    if (huff == NULL) {
        printf("Erro ao abrir o arquivo compactado!\n");
        return 1;
    }

    int byte1 = fgetc(huff);
    int byte2 = fgetc(huff);
    if (byte1 == EOF || byte2 == EOF) {
        printf("Arquivo .huff invalido!\n");
        fclose(huff);
        return 1;
    }
    int lixo = byte1 >> 5;
    int tam_arvore = ((byte1 & 0x1F) << 8) | byte2;

    int restantes = tam_arvore;
    NoArvore *raiz = reconstruir_arvore(huff, &restantes);

    FILE *saida = fopen(nome_saida, "wb");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida!\n");
        arvore_liberar(raiz);
        fclose(huff);
        return 1;
    }

    if (raiz != NULL) {
        NoArvore *atual = raiz;
        int byte_atual = fgetc(huff);

        while (byte_atual != EOF) {
            int proximo = fgetc(huff);
            int ultimo_bit = (proximo == EOF) ? lixo : 0;

            for (int pos = 7; pos >= ultimo_bit; pos--) {
                if (is_bit_i_set((unsigned char)byte_atual, pos)) {
                    atual = atual->dir;
                } else {
                    atual = atual->esq;
                }

                if (atual == NULL) {
                    printf("Arquivo .huff corrompido!\n");
                    arvore_liberar(raiz);
                    fclose(huff);
                    fclose(saida);
                    return 1;
                }

                if (arvore_eh_folha(atual)) {
                    fputc(arvore_byte(atual), saida);
                    atual = raiz;
                }
            }
            byte_atual = proximo;
        }
    }

    arvore_liberar(raiz);
    fclose(huff);
    fclose(saida);
    printf("Descompactacao concluida com sucesso!\n");
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) return compactar_arquivo(argv[2], argv[3]);
        if (strcmp(argv[1], "-d") == 0) return descompactar_arquivo(argv[2], argv[3]);
        printf("Opcao invalida. Use -c ou -d.\n");
        return 1;
    }

    int opcao;
    char origem[256], destino[256];

    printf("1 - Compactar arquivo\n2 - Descompactar arquivo\nEscolha: ");
    if (scanf("%d", &opcao) != 1) return 1;

    printf("Arquivo de entrada: ");
    if (scanf("%255s", origem) != 1) return 1;

    printf("Arquivo de saida: ");
    if (scanf("%255s", destino) != 1) return 1;

    if (opcao == 1) return compactar_arquivo(origem, destino);
    if (opcao == 2) return descompactar_arquivo(origem, destino);

    printf("Opcao invalida.\n");
    return 1;
}