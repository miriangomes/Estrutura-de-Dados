#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> 

#define MAX_CLAUSULAS 10000
#define MAX_LITERAIS 1000
#define MAX_VARS 1000 


typedef int Clausula[MAX_LITERAIS];


int num_clausulas = 0;
int num_variaveis = 0; 

Clausula MATRIZ_VAR[MAX_CLAUSULAS];
int atribuicao[MAX_VARS + 1];


void limpar_atribuicao() {
   
    for (int i = 0; i <= MAX_VARS; i++) {
        atribuicao[i] = 0;
    }
}

bool clausula_satisfeita(const Clausula clausula) {
    for (int i = 0; clausula[i] != 0; i++) {
        int literal = clausula[i];
        int indice_var = abs(literal);
        
        int valor = atribuicao[indice_var];
        
        if (valor == 0) continue; 
        
        if (literal > 0 && valor == 1) return true; 
        if (literal < 0 && valor == -1) return true; 
    }
    return false;
}

bool formula_satisfeita() {
    for (int i = 0; i < num_clausulas; i++) {
        if (!clausula_satisfeita(MATRIZ_VAR[i])) {
            return false;
        }
    }
    return true;
}

bool clausula_falsificada() {
    for (int i = 0; i < num_clausulas; i++) {
        bool literal_pode_ser_verdadeiro = false;
        
        for (int j = 0; MATRIZ_VAR[i][j] != 0; j++) {
            int literal = MATRIZ_VAR[i][j];
            int indice_var = abs(literal);
            int valor = atribuicao[indice_var];
            
            // Se não houve uma verificação prévia, a clausula ainda pode ser verdadeira
            if (valor == 0) {
                literal_pode_ser_verdadeiro = true;
                break;
            }
                                          
            // Verifica se essa clausula ainda tem chance de ser verdadeira com a atribuição atual
            if (literal > 0 && valor == 1) { 
                literal_pode_ser_verdadeiro = true;
                break;
            }

            if (literal < 0 && valor == -1) {
                literal_pode_ser_verdadeiro = true;
                break;
            }
        }
        
        // Se, após checar todos os literais, esta cláusula (i) não pôde ser verdadeira, a formula inteira falhou (backtracking)
        if (literal_pode_ser_verdadeiro == false) { 
            return true;
        }
    }

    return false; // Caso contrário, a clausula ainda pode ser verdadeira
}

int encontrar_proxima_var_nao_atribuida() {
   
    for (int i = 1; i <= num_variaveis; i++) {
        if (atribuicao[i] == 0) {
            return i;
        }
    }
    return 0; 
}

bool resolver_dpll() {
    if (formula_satisfeita()) {
        return true; 
    }
    if (clausula_falsificada()) {
        return false; 
    }

    int var = encontrar_proxima_var_nao_atribuida();
    if (var == 0) {
        
        return false; 
    }

    // Tenta var = Verdadeiro (1)
    atribuicao[var] = 1;
    if (resolver_dpll()) {
        return true;
    }

    // Tenta var = Falso (-1)
    atribuicao[var] = -1;
    if (resolver_dpll()) {
        return true;
    }

    // Backtrack
    atribuicao[var] = 0;
    return false;
}


int main(int argc, char *argv[]) {
    FILE *input_stream = stdin;

    if (argc > 2) {
        fprintf(stderr, "Uso: %s [arquivo.cnf]\n", argv[0]);
        return 1;
    }
    
    if (argc == 2) {
      
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("Erro ao abrir arquivo");
            return 1;
        }
    }

    
    
    limpar_atribuicao();

    char c;

   
    while ((c = fgetc(input_stream)) == 'c') {

    while ((c = fgetc(input_stream)) != '\n' && c != EOF);
    }

    if (c != EOF) {
        ungetc(c, input_stream);
    }

    
    if (fscanf(input_stream, "p cnf %d %d", &num_variaveis, &num_clausulas) != 2) {
        fprintf(stderr, "Erro: Nao foi possivel ler a linha 'p cnf' do formato DIMACS.\n");
        if (input_stream != stdin) fclose(input_stream);
        return 1;
    }

    // Verifica se os limites definidos são suficientes
    if (num_variaveis > MAX_VARS) {
        fprintf(stderr, "Erro: O numero de variaveis (%d) excede o limite MAX_VARS (%d).\n", num_variaveis, MAX_VARS);
        if (input_stream != stdin) fclose(input_stream);
        return 1;
    }
    if (num_clausulas > MAX_CLAUSULAS) {
        fprintf(stderr, "Erro: O numero de clausulas (%d) excede o limite MAX_CLAUSULAS (%d).\n", num_clausulas, MAX_CLAUSULAS);
        if (input_stream != stdin) fclose(input_stream);
        return 1;
    }

   
    while ((c = fgetc(input_stream)) != '\n' && c != EOF); 
    
   
    for (int i = 0; i < num_clausulas; i++) {
        int literal;
        int j = 0;
       
        while (fscanf(input_stream, "%d", &literal) == 1 && literal != 0) {
            if (j < MAX_LITERAIS - 1) { 
                MATRIZ_VAR[i][j] = literal;
                j++;
            } else {
                 fprintf(stderr, "Aviso: Clausula %d excede MAX_LITERAIS. Ignorando literais extras.\n", i+1);
                
            }
        }
        MATRIZ_VAR[i][j] = 0;
    }

    
    if (input_stream != stdin) {
        fclose(input_stream);
    }

    
    printf("Iniciando o SAT Solver (vars=%d, clausulas=%d)...\n", num_variaveis, num_clausulas);
    if (resolver_dpll()) {
        printf("SAT!\n");
       
        printf("Atribuicao:\n");
        for (int i = 1; i <= num_variaveis; i++) {
            printf("Var %d = %d\n", i, atribuicao[i]);
        }
    } else {
        printf("UNSAT.\n");
    }

    return 0;
}