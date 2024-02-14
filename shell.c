#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/limits.h>

const int marimeDeBaza = 500;
char *input, cwd[PATH_MAX];
size_t input_size = 0;

long long bufferFisier(FILE *f);
bool esteScript(char *comanda);
char **separaDupaSpatii(char rand[]);
void executare_comanda(char** comanda);
void executa_comenzi_script(FILE *f, long long buffer, char **comandaInitiala);
void creareVariabile(char **comanda, char **comandaInitiala);
void changeDirectory(char **argumente);
void buclaShell();
bool doarSpatii(char *s);

int main(int argc, char** argv) {

    buclaShell();

    free(input);
    return EXIT_SUCCESS;
}


//Functia determina lungimea maxima a unei linii dintr-un fisier
long long bufferFisier(FILE *f) {

    long long maxim = 0, nr = 0;
    char c;

    while(fscanf(f, "%c", &c) != EOF) {

        nr++;
        if(nr > maxim)
            maxim = nr;
        if(c == '\n')
            nr = 0;
    }

    return maxim;
}

//Functia verifica daca este un script
bool esteScript(char *comanda) {

    char *dupComanda = strdup(comanda);
    char *primulArgument = strtok(dupComanda, " \t");

    if(primulArgument != NULL && access(primulArgument, F_OK) != -1) {

        free(dupComanda);
        return true;
    }

    free(dupComanda);
    return false;
}

//Functia separa comanda in cuvinte
char **separaDupaSpatii(char rand[]) {

    int marimeCitita = marimeDeBaza, poz = 0;
    char *token;
    char **tokenuri = (char**)malloc(marimeCitita * sizeof(char*));

    if(tokenuri == NULL) {

        fprintf(stderr, "Eroare alocare token-uri. \n");
        exit(EXIT_FAILURE);
    }

    token = strtok(rand, " \t\'\"");

    while(token != NULL) {

        tokenuri[poz++] = token;

        if(poz >= marimeCitita) {

            marimeCitita += marimeDeBaza;
            tokenuri = (char**)realloc(tokenuri, marimeCitita * sizeof(char*));
            if(tokenuri == NULL) {
                fprintf(stderr, "Eroare realocare token-uri. \n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\'\"");
    }

    tokenuri[poz] = NULL;
    return tokenuri;
}

//Functia executa comanda
void executare_comanda(char** comanda) {

    if(strcmp(comanda[0], "cd") == 0)
        changeDirectory(comanda);
    else {

    pid_t pid = fork();
    if(pid == 0) {

        execvp(comanda[0], comanda);
        perror("Eroare");
        exit(EXIT_FAILURE);
    }
    else 
    if(pid > 0)
        wait(NULL);
    else 
        perror("Fork esuat.");
    }
}

//Functia executa comenzile din script
void executa_comenzi_script(FILE *f, long long buffer, char **comandaInitiala) {

    char *rand, **comandaSeparata;
    rand = (char*)malloc((buffer + 1) * sizeof(char));
    rewind(f);

    while(fgets(rand, buffer+1, f)) {

        if(strcmp(rand, "\n") == 0)
            continue;

        if(rand[strlen(rand)-1] == '\n')
            rand[strlen(rand)-1] = '\0';

        comandaSeparata = separaDupaSpatii(rand);
        if(comandaSeparata == NULL) {
            fprintf(stderr, "Eroare la separarea comenzii din script. \n");
            exit(EXIT_FAILURE);
        }

        if(strcmp(comandaSeparata[0], "echo") == 0)
            creareVariabile(comandaSeparata, comandaInitiala);

        executare_comanda(comandaSeparata);
    }

    free(rand);
}

//Functia creaza variabilele in functie de $
void creareVariabile(char **comanda, char **comandaInitiala) {

    for(int i = 1; comanda[i] != NULL; ++i) {

        if(comanda[i][0] == '$') {

            char *endptr;
            long int indexVar = strtol(comanda[i] + 1, &endptr, 10);

            if(comanda[i] + 1 != endptr) {
            
                if(indexVar >= 0) {

                    if(comandaInitiala[indexVar] != NULL)
                        comanda[i] = strdup(comandaInitiala[indexVar]);
                    else
                        comanda[i] = strdup("");
                }
                else {
                
                    fprintf(stderr, "IndexVar invalid. \n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

//Functia simuleaza comanda cd
void changeDirectory(char **argumente) {

    if(argumente[1] == NULL) 
        fprintf(stderr, "Utilizare: cd \"director\" \n");
    else {

        char *path;
        path = strdup("");

        for(int i = 1; argumente[i] != NULL; ++i) {

            strcat(path, argumente[i]);
            if(argumente[i + 1] != NULL)
                strcat(path, " ");
        }

        if(path[0] == '\"' && path[strlen(path) - 1] == '\"') {

            path[strlen(path) - 1] = '\0';
            path++;
        }

        if(chdir(path) != 0) {
            free(path);
            perror("cd");
            return;
        }

        free(path);
    }
}

//Functia ruleaza shell-ul
void buclaShell() {

    while(true) {

        if(getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("eroare getcwd()");
            break;
        }

        printf("%s $> ", cwd);
        fflush(stdout);
        
        if(getline(&input, &input_size, stdin) == -1)
            break;

        if(strcmp(input, "\n") == 0 || doarSpatii(input))
            continue;

        if(input[strlen(input) - 1] == '\n')
            input[strlen(input) - 1] = '\0';

        if(strcmp(input, "exit") == 0) {
            printf("Iesire din shell. \n");
            break;
        }

        if(esteScript(input)) {

            char *primulArgument = strtok(strdup(input), " \t");
            FILE *f = fopen(primulArgument, "r");
            if(f == NULL) {
                perror(primulArgument);
                break;
            }
            long long buffer = bufferFisier(f);
            char **comandaInitiala;
            comandaInitiala = separaDupaSpatii(input);
            executa_comenzi_script(f, buffer, comandaInitiala);
        }
        else {

            char **inputSeparat = separaDupaSpatii(input);
            executare_comanda(inputSeparat);
        }
    }    
}

//Functia verifica daca un string contine doar spatii
bool doarSpatii(char *s) {

    for(int i = 0; s[i] != '\0'; ++i)
        if(s[i] != ' ' && s[i] != '\n')
            return false;

    return true;
}