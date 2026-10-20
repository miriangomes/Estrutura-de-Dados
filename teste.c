/**
 * @file huffman.c
 * @brief Compactador/descompactador de arquivos usando Codificacao de Huffman.
 *
 * Funciona com QUALQUER tipo de arquivo (texto, imagem, pdf...) porque
 * trabalha com BYTES (0 a 255) e nao com "letras".
 *
 * Formato do arquivo .huff (igual ao da Aula 20):
 *   [3 bits de lixo][13 bits de tamanho da arvore][arvore em pre-ordem][dados]
 *
 * Compilar:  gcc -Wall -Wextra -o huffman huffman.c
 * Usar:      ./huffman -c original.txt saida.huff
 *            ./huffman -d saida.huff recuperado.txt
 *
 * =====================================================================
 *  COLA RAPIDA DE PONTEIROS (leia isso primeiro!)
 * =====================================================================
 *  int x = 5;        -> "x" e uma caixinha que guarda o numero 5.
 *  int *p = &x;      -> "p" e uma caixinha que guarda o ENDERECO de x.
 *                       (& = "me da o endereco de")
 *  *p = 10;          -> vai ao endereco guardado em p e escreve 10 la.
 *                       (* = "va ate o endereco e mexa no que esta la")
 *  no->esq           -> igual a (*no).esq : "no e um endereco de struct,
 *                       me da o campo esq da struct que mora la".
 *  malloc(n)         -> pede ao computador n bytes de memoria. Devolve o
 *                       endereco (ou NULL se nao tiver memoria).
 *  free(p)           -> devolve essa memoria. Tudo que tem malloc precisa
 *                       de um free, senao a memoria "vaza".
 *  NULL              -> "endereco nenhum" (ponteiro que nao aponta pra nada).
 *  void *            -> ponteiro "sem tipo": guarda um endereco de QUALQUER
 *                       coisa. Para USAR o que tem la, voce precisa dizer
 *                       o tipo com um "cast": *(int *)p  = "trate p como
 *                       endereco de int e leia o int".
 * =====================================================================
 */

#include <stdio.h>   /* printf, fopen, fgetc, fputc, fclose...   */
#include <stdlib.h>  /* malloc, free, exit                       */
#include <string.h>  /* strcpy, strlen, strcmp                   */

#define TAM_ALFABETO 256   /* existem 256 bytes possiveis (0 a 255)       */
#define TAM_MAX_CODIGO 256 /* tamanho maximo (em chars) de um codigo      */


/* ====================================================================
 *  UTILITARIO: malloc "seguro"
 * ==================================================================== */

/**
 * @brief Confere se um malloc deu certo. Se nao deu, encerra o programa.
 * @param ptr Endereco devolvido pelo malloc.
 * @return O mesmo endereco (para podermos usar em uma linha so).
 */
static void *checar_alocacao(void *ptr) {
    if (ptr == NULL) {                       /* malloc devolve NULL se falhar */
        fprintf(stderr, "Erro: sem memoria!\n");
        exit(1);                             /* encerra o programa com erro   */
    }
    return ptr;                              /* deu certo: devolve o endereco */
}


/* ====================================================================
 *  TAD 1: ARVORE BINARIA (usa void* para guardar os dados)
 * ==================================================================== */

/**
 * @brief No da arvore de Huffman.
 *
 * Os DADOS (byte e frequencia) ficam em void*, como o professor exige.
 * Os filhos sao ponteiros normais para outros nos.
 */
typedef struct NoArvore {
    void *byte;               /* aponta para um unsigned char (o byte)     */
    void *frequencia;         /* aponta para um long long (qtd. de vezes)  */
    struct NoArvore *esq;     /* filho da esquerda  (caminho = bit 0)      */
    struct NoArvore *dir;     /* filho da direita   (caminho = bit 1)      */
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
    /* 1) reserva memoria para a struct inteira (4 ponteiros)               */
    NoArvore *no = checar_alocacao(malloc(sizeof(NoArvore)));

    /* 2) reserva memoria para guardar o byte e a frequencia                */
    no->byte       = checar_alocacao(malloc(sizeof(unsigned char)));
    no->frequencia = checar_alocacao(malloc(sizeof(long long)));

    /* 3) escreve os valores la dentro.                                      */
    /*    (unsigned char *)no->byte  = "trate esse void* como unsigned char*"*/
    /*    o * da frente = "va ate la e escreva"                              */
    *(unsigned char *)no->byte       = byte;
    *(long long *)no->frequencia     = freq;

    /* 4) liga os filhos                                                     */
    no->esq = esq;
    no->dir = dir;
    return no;
}

/** @brief Le o byte guardado em um no. */
unsigned char arvore_byte(const NoArvore *no) {
    return *(unsigned char *)no->byte;       /* cast + "va ate la e leia" */
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
    if (raiz == NULL) return;        /* caso base: nada a liberar            */
    arvore_liberar(raiz->esq);       /* primeiro libera a subarvore esquerda */
    arvore_liberar(raiz->dir);       /* depois a direita                     */
    free(raiz->byte);                /* libera o que o void* apontava...     */
    free(raiz->frequencia);
    free(raiz);                      /* ...e por ultimo o proprio no         */
}


/* ====================================================================
 *  TAD 2: FILA DE PRIORIDADE GENERICA (void*)
 * ==================================================================== */

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
    void *item;                 /* o que esta guardado (qualquer coisa)     */
    struct NoFila *proximo;     /* proximo da fila                          */
} NoFila;

/** @brief Fila de prioridade (lista encadeada ORDENADA, menor primeiro). */
typedef struct {
    NoFila *inicio;             /* primeiro da fila (o de menor valor)      */
    int tamanho;                /* quantos itens tem na fila                */
} Fila;

/** @brief Cria uma fila vazia. */
Fila *fila_criar(void) {
    Fila *fila = checar_alocacao(malloc(sizeof(Fila)));
    fila->inicio = NULL;        /* fila vazia: ninguem no inicio            */
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

    /* Caso 1: fila vazia OU o novo e menor/igual ao primeiro -> vai pro inicio */
    if (fila->inicio == NULL || comparar(item, fila->inicio->item) <= 0) {
        novo->proximo = fila->inicio;   /* novo aponta para o antigo primeiro */
        fila->inicio = novo;            /* e passa a ser o primeiro           */
    } else {
        /* Caso 2: anda ate achar o ultimo no que ainda e MENOR que o novo    */
        NoFila *atual = fila->inicio;
        while (atual->proximo != NULL &&
               comparar(atual->proximo->item, item) < 0) {
            atual = atual->proximo;
        }
        /* encaixa o novo logo depois de "atual"                              */
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
    if (fila->inicio == NULL) return NULL;   /* nada para remover             */

    NoFila *removido = fila->inicio;         /* guarda o primeiro             */
    void *item = removido->item;             /* guarda o que tinha dentro     */
    fila->inicio = removido->proximo;        /* o segundo vira o primeiro     */
    free(removido);                          /* libera so a "caixinha" da fila*/
    fila->tamanho--;
    return item;                             /* o item continua vivo!         */
}

/** @brief Libera a fila (so chame com a fila vazia). */
void fila_liberar(Fila *fila) {
    free(fila);
}


/* ====================================================================
 *  TAD 3: OPERACOES COM BITS (Slides "Is bit i set?" e "Setting a bit")
 * ==================================================================== */

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
    unsigned char mask = 1 << i;   /* cria 00..1..00 com o 1 na posicao i   */
    return mask & c;               /* E bit-a-bit: so sobra o bit i         */
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
    unsigned char mask = 1 << i;   /* mesma mascara                         */
    return mask | c;               /* OU bit-a-bit: forca aquele bit a 1    */
}


/* ====================================================================
 *  PASSO 1: CONTAR FREQUENCIAS
 * ==================================================================== */

/**
 * @brief Conta quantas vezes cada byte (0..255) aparece no arquivo.
 * @param arquivo Arquivo aberto em modo binario ("rb").
 * @param freq    Vetor de 256 posicoes; freq[65] = quantos 'A' (65 = 'A').
 */
void contar_frequencias(FILE *arquivo, long long freq[TAM_ALFABETO]) {
    int c;                                    /* int (e nao char) por causa do EOF */
    while ((c = fgetc(arquivo)) != EOF) {     /* le 1 byte; EOF = acabou o arquivo */
        freq[c]++;                            /* o proprio byte e o indice!        */
    }
}


/* ====================================================================
 *  PASSO 2: CONSTRUIR A ARVORE DE HUFFMAN
 * ==================================================================== */

/**
 * @brief Compara dois nos de arvore pela frequencia (usada pela fila).
 */
int comparar_nos(const void *a, const void *b) {
    long long fa = arvore_frequencia((const NoArvore *)a);   /* cast: void* -> NoArvore* */
    long long fb = arvore_frequencia((const NoArvore *)b);
    return (fa > fb) - (fa < fb);   /* devolve 1, 0 ou -1 (sem risco de overflow) */
}

/**
 * @brief Monta a arvore de Huffman a partir das frequencias.
 * @return Raiz da arvore (NULL se o arquivo era vazio).
 */
NoArvore *construir_arvore(long long freq[TAM_ALFABETO]) {
    Fila *fila = fila_criar();

    /* 1) uma folha para cada byte que apareceu, na fila ordenada            */
    for (int i = 0; i < TAM_ALFABETO; i++) {
        if (freq[i] > 0) {
            NoArvore *folha = arvore_criar_no((unsigned char)i, freq[i], NULL, NULL);
            fila_inserir(fila, folha, comparar_nos);
        }
    }

    /* 2) arquivo vazio: nao ha arvore                                       */
    if (fila->tamanho == 0) {
        fila_liberar(fila);
        return NULL;
    }

    /* 3) arquivo com UM unico tipo de byte (ex: "AAAA"): a folha sozinha    */
    /*    ficaria com codigo vazio. Colocamos um pai para ela ter o codigo "0"*/
    if (fila->tamanho == 1) {
        NoArvore *unica = fila_remover(fila);
        fila_liberar(fila);
        return arvore_criar_no('*', arvore_frequencia(unica), unica, NULL);
    }

    /* 4) enquanto houver 2 ou mais: junta os dois menores num pai            */
    while (fila->tamanho > 1) {
        NoArvore *esq = fila_remover(fila);    /* menor frequencia            */
        NoArvore *dir = fila_remover(fila);    /* segunda menor               */
        long long soma = arvore_frequencia(esq) + arvore_frequencia(dir);
        NoArvore *pai = arvore_criar_no('*', soma, esq, dir);
        fila_inserir(fila, pai, comparar_nos); /* devolve o pai pra fila      */
    }

    NoArvore *raiz = fila_remover(fila);       /* o ultimo que sobrou e a raiz */
    fila_liberar(fila);
    return raiz;
}


/* ====================================================================
 *  PASSO 3: TABELA DE CODIGOS (A=11, B=01, ...)
 * ==================================================================== */

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
    if (no == NULL) return;                    /* caso base                   */

    if (arvore_eh_folha(no)) {
        caminho[profundidade] = '\0';          /* fecha o texto               */
        strcpy(codigos[arvore_byte(no)], caminho); /* copia para a linha do byte */
        return;
    }

    caminho[profundidade] = '0';               /* vai pela esquerda           */
    gerar_codigos(no->esq, codigos, caminho, profundidade + 1);

    caminho[profundidade] = '1';               /* vai pela direita            */
    gerar_codigos(no->dir, codigos, caminho, profundidade + 1);
}


/* ====================================================================
 *  PASSO 4: CABECALHO (lixo + tamanho da arvore + arvore)
 * ==================================================================== */

/**
 * @brief Calcula quantos bits de LIXO sobram no ultimo byte (0 a 7).
 */
int calcular_lixo(long long freq[TAM_ALFABETO],
                  char codigos[TAM_ALFABETO][TAM_MAX_CODIGO]) {
    long long total_bits = 0;
    for (int i = 0; i < TAM_ALFABETO; i++) {
        /* cada byte aparece freq[i] vezes e ocupa strlen(codigo) bits       */
        total_bits += freq[i] * (long long)strlen(codigos[i]);
    }
    int resto = total_bits % 8;        /* bits usados no ultimo byte          */
    return (resto == 0) ? 0 : 8 - resto; /* o que falta pra fechar 8 e lixo   */
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
            fputc('\\', saida);        /* escape: "o proximo e folha de verdade" */
        }
        fputc(c, saida);               /* escreve o byte da folha                */
        return;
    }

    fputc('*', saida);                 /* no interno                             */
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
    /* lixo << 5 : empurra os 3 bits do lixo para as 3 posicoes da esquerda  */
    /* tam_arvore >> 8 : pega so os 5 bits mais altos dos 13 bits            */
    unsigned char byte1 = (lixo << 5) | (tam_arvore >> 8);
    /* & 0xFF : fica so com os 8 bits mais baixos                            */
    unsigned char byte2 = tam_arvore & 0xFF;

    fputc(byte1, saida);
    fputc(byte2, saida);
    escrever_arvore(raiz, saida);
}


/* ====================================================================
 *  COMPACTAR
 * ==================================================================== */

/**
 * @brief Compacta um arquivo qualquer.
 * @param nome_entrada Arquivo original.
 * @param nome_saida   Arquivo .huff a ser criado.
 * @return 0 se deu certo, 1 se deu erro.
 */
int compactar_arquivo(const char *nome_entrada, const char *nome_saida) {
    FILE *entrada = fopen(nome_entrada, "rb");   /* "rb" = leitura BINARIA     */
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada!\n");
        return 1;
    }

    /* 1) frequencias */
    long long freq[TAM_ALFABETO] = {0};          /* tudo comeca em zero        */
    contar_frequencias(entrada, freq);

    /* 2) arvore */
    NoArvore *raiz = construir_arvore(freq);

    /* 3) tabela de codigos */
    char codigos[TAM_ALFABETO][TAM_MAX_CODIGO] = {{0}};
    char caminho[TAM_MAX_CODIGO];
    gerar_codigos(raiz, codigos, caminho, 0);

    /* 4) lixo e tamanho da arvore */
    int lixo = calcular_lixo(freq, codigos);
    int tam_arvore = calcular_tamanho_arvore(raiz);

    /* 5) abre a saida e escreve o cabecalho */
    FILE *saida = fopen(nome_saida, "wb");       /* "wb" = escrita BINARIA     */
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida!\n");
        arvore_liberar(raiz);
        fclose(entrada);
        return 1;
    }
    escrever_cabecalho(saida, lixo, tam_arvore, raiz);

    /* 6) volta ao inicio do original e escreve os bits compactados */
    rewind(entrada);

    unsigned char byte_atual = 0;   /* byte que estamos montando bit a bit     */
    int pos = 7;                    /* proximo bit a preencher (7 = esquerda)  */
    int c;

    while ((c = fgetc(entrada)) != EOF) {
        const char *codigo = codigos[c];         /* ex: "1001"                 */

        for (int j = 0; codigo[j] != '\0'; j++) {
            if (codigo[j] == '1') {
                byte_atual = set_bit(byte_atual, pos);  /* '0' ja e zero, nao faz nada */
            }
            pos--;                               /* anda um bit pra direita    */

            if (pos < 0) {                       /* encheu 8 bits = 1 byte     */
                fputc(byte_atual, saida);        /* grava                      */
                byte_atual = 0;                  /* recomeca                   */
                pos = 7;
            }
        }
    }

    if (pos != 7) {                  /* sobrou byte incompleto (tem lixo)      */
        fputc(byte_atual, saida);    /* grava com zeros no final               */
    }

    arvore_liberar(raiz);
    fclose(entrada);
    fclose(saida);
    printf("Compactacao concluida com sucesso!\n");
    return 0;
}


/* ====================================================================
 *  DESCOMPACTAR
 * ==================================================================== */

/**
 * @brief Reconstroi a arvore lendo o arquivo em pre-ordem.
 * @param huff      Arquivo .huff, posicionado no inicio da arvore.
 * @param restantes Quantos bytes da arvore ainda faltam ler (passado por
 *                  ENDERECO, para que todas as chamadas recursivas
 *                  enxerguem e atualizem o mesmo contador).
 * @return Raiz (sub)arvore lida.
 */
NoArvore *reconstruir_arvore(FILE *huff, int *restantes) {
    if (*restantes <= 0) return NULL;       /* acabou a arvore                 */

    int c = fgetc(huff);                    /* le 1 byte da arvore             */
    if (c == EOF) return NULL;              /* arquivo corrompido              */
    (*restantes)--;                         /* (*restantes) = "o valor la"     */

    if (c == '*') {                         /* no interno: le esq e dir        */
        NoArvore *pai = arvore_criar_no('*', 0, NULL, NULL);
        pai->esq = reconstruir_arvore(huff, restantes);
        pai->dir = reconstruir_arvore(huff, restantes);
        return pai;
    }

    if (c == '\\') {                        /* escape: o proximo e folha real  */
        c = fgetc(huff);
        (*restantes)--;
    }

    return arvore_criar_no((unsigned char)c, 0, NULL, NULL);   /* folha        */
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

    /* 1) le os 2 primeiros bytes */
    int byte1 = fgetc(huff);
    int byte2 = fgetc(huff);
    if (byte1 == EOF || byte2 == EOF) {
        printf("Arquivo .huff invalido!\n");
        fclose(huff);
        return 1;
    }
    int lixo = byte1 >> 5;                          /* 3 bits da esquerda      */
    int tam_arvore = ((byte1 & 0x1F) << 8) | byte2; /* 5 bits + 8 bits = 13    */
    /* 0x1F = 00011111: o & apaga os 3 bits do lixo e deixa os outros 5        */

    /* 2) reconstroi a arvore */
    int restantes = tam_arvore;
    NoArvore *raiz = reconstruir_arvore(huff, &restantes);  /* & = passa o endereco */

    FILE *saida = fopen(nome_saida, "wb");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida!\n");
        arvore_liberar(raiz);
        fclose(huff);
        return 1;
    }

    /* 3) le os bytes de dados e anda pela arvore bit a bit.                   */
    /*    Truque: sempre lemos UM BYTE A FRENTE. Se o proximo for EOF, o byte  */
    /*    atual e o ULTIMO e devemos parar antes dos bits de lixo.             */
    if (raiz != NULL) {                              /* NULL = arquivo vazio   */
        NoArvore *atual = raiz;
        int byte_atual = fgetc(huff);

        while (byte_atual != EOF) {
            int proximo = fgetc(huff);
            int ultimo_bit = (proximo == EOF) ? lixo : 0;  /* ate onde ler     */

            for (int pos = 7; pos >= ultimo_bit; pos--) {
                if (is_bit_i_set((unsigned char)byte_atual, pos)) {
                    atual = atual->dir;              /* bit 1 -> direita       */
                } else {
                    atual = atual->esq;              /* bit 0 -> esquerda      */
                }

                if (atual == NULL) {                 /* caminho invalido       */
                    printf("Arquivo .huff corrompido!\n");
                    arvore_liberar(raiz);
                    fclose(huff);
                    fclose(saida);
                    return 1;
                }

                if (arvore_eh_folha(atual)) {        /* achou um byte!         */
                    fputc(arvore_byte(atual), saida);
                    atual = raiz;                    /* volta pra raiz         */
                }
            }
            byte_atual = proximo;                    /* avanca                 */
        }
    }

    arvore_liberar(raiz);
    fclose(huff);
    fclose(saida);
    printf("Descompactacao concluida com sucesso!\n");
    return 0;
}


/* ====================================================================
 *  MAIN
 * ==================================================================== */

int main(int argc, char *argv[]) {
    /* Modo terminal: ./huffman -c entrada saida   ou   -d entrada saida     */
    if (argc == 4) {
        if (strcmp(argv[1], "-c") == 0) return compactar_arquivo(argv[2], argv[3]);
        if (strcmp(argv[1], "-d") == 0) return descompactar_arquivo(argv[2], argv[3]);
        printf("Opcao invalida. Use -c ou -d.\n");
        return 1;
    }

    /* Modo menu */
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