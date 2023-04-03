#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

struct section{
    char sect_name[9];
    unsigned char sect_type;
    unsigned int sect_offset;
    unsigned int sect_size; 
};

struct Header{
    unsigned char version;
    struct section* sections;
    unsigned char no_of_sections;
    unsigned short header_size;
};

#define MAX_PATH 512

int listLiniar(char * path, int condType, char* cond){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        printf("ERROR");
        printf("\ninvalid directory path");
        return -1;
    }

    printf("SUCCESS");
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) { //daca numele fisierului este diferit de .(folderul curent) si de ..(folderul parinte)
            snprintf(fullPath, MAX_PATH, "%s/%s", path, entry->d_name); //cream path-ul complet din pathul directorului in care ne aflam si numele fisierului curent

            if(lstat(fullPath, &statbuf) == 0) { //cream statbuf cu informatii despre fisier

                if(condType == 0){// condType=0 ==> fara filtre
                        printf("\n%s", fullPath);
                } else {
                    if(condType==1 ){
                        if((strstr(entry->d_name, cond) == &entry->d_name[0] && strlen(entry->d_name) >= strlen(cond)) ){// daca strstr returneaza pointer la inceputul numelui fisierului si dim sirului cautat este ,ai mica decat dimensiunea numelui fisierului
                            printf("\n%s", fullPath);
                        }
                    } else {
                        if(condType==2){ //condType=2 ==> permissions
                            int mode = 0;
                            for(int i=0; i<9; i++){  //calculam valoarea permisiunilor primite ca argument si o comparam cu cea a fisierului
                                mode *=2;
                                if(cond[i] == 'r' || cond[i] == 'w' || cond[i] == 'x'){
                                    mode ++;
                                } else {
                                    if(cond[i] != '-'){
                                        printf("ERROR\nCaractere invalide la permisiuni");
                                    }
                                }
                                if((statbuf.st_mode & 0777) == mode){
                                    printf("\n%s", fullPath);
                                }
                            }
                        }
                    }
                }     
            }
        }
        
    }   
    closedir(dir);
    return 0;
}

int listRecursive(char* path, int condType, char* cond, int* time){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;
    
    if(time == 0){
        printf("SUCCESS\n");
        time++;
    }

    dir = opendir(path);
    if(dir == NULL) {
        printf("ERROR\ninvalid directory path");
        return -1;
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { //daca numele fisierului este diferit de .(folderul curent) si de ..(folderul parinte)
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name); //cream path-ul complet din pathul directorului in care ne aflam si numele fisierului curent
            if(lstat(fullPath, &statbuf) == 0) { //cream statbuf cu informatii despre fisier

                if(condType == 0){// condType=0 ==> fara filtre
                    printf("\n%s", fullPath);
                } else {
                    if(condType==1 ){
                        if((strstr(entry->d_name, cond) == &entry->d_name[0] && strlen(entry->d_name) >= strlen(cond)) ){// daca strstr returneaza pointer la inceputul numelui fisierului si dim sirului cautat este ,ai mica decat dimensiunea numelui fisierului
                            printf("\n%s", fullPath);
                        }
                    } else {
                        if(condType==2){ //condType=2 ==> permissions
                            int mode = 0;
                            for(int i=0; i<9; i++){  //calculam valoarea permisiunilor primite ca argument si o comparam cu cea a fisierului
                                mode *=2;
                                if(cond[i] == 'r' || cond[i] == 'w' || cond[i] == 'x'){
                                    mode ++;
                                } else {
                                    if(cond[i] != '-'){
                                        printf("ERROR\nCaractere invalide la permisiuni");
                                    }
                                }
                            }
                            if((statbuf.st_mode & 0777) == mode){
                                printf("\n%s", fullPath);
                            }
                        }
                    }
                    
                }
                
                if(S_ISDIR(statbuf.st_mode)) { //apelam recursiv daca fisierul curent este un director
                    listRecursive(fullPath, condType, cond, time);
                }
            }
        }
    }
    closedir(dir);
    return 0;
}



void list(char* path, int recursive, int condType, char* cond){
    if(recursive == 1){  //apelam functia de list potrivita in functie de parametrul recursive
        printf("SUCCESS");
        int time = 0;
        listRecursive(path, condType, cond, &time);
        printf("\n");
    } else {
        listLiniar(path, condType, cond);
    }
}



struct Header * parse(char* path){ //functia de parse returneaza pointer la o structura header care contine toate datele din header
    int fd = open(path, O_RDONLY);  //descidem fisierul
    if(-1 == fd){
        printf("ERROR\nnu se poate deschide fisierul");
        return NULL;
    }
    lseek(fd, -1, SEEK_END); //pozitionam cursorul la sfarsit pentru a putea citi magic-ul
    char magic;
    if( read(fd, &magic, 1) != 1){ //citim si verificam magic
        printf("ERROR\neroare la citirea din fisier -- magic");
        close(fd);
        return NULL;
    }
    if(magic != 'Q'){
        printf("ERROR\nwrong magic");
        close(fd);
        return NULL;
    }

    struct Header* header = (struct Header*) malloc (sizeof(struct Header) * 1); // alocam spatiu pentru header

    lseek(fd, -3, SEEK_END); //pozitionam cursorul pentru a putea citi header size
    if(read(fd, &header->header_size , 2) != 2){  //citim header size
        printf("ERROR\neroare la citirea din fisier -- header size");
        free(header);
        close(fd);
        return NULL;
    }

    lseek(fd, -1*header->header_size, SEEK_END); //pozitionam cursorul la inceputul headerului

    if( read(fd, &header->version, 1) != 1){// citim si verificam versiunea
        printf("ERROR\neroare la citirea din fisier -- version");
        free(header);
        close(fd);
        return NULL;
    }
    if(header->version < 113 || header->version > 163){
        printf("ERROR\nwrong version");
        free(header);
        close(fd);
        return NULL;
    }

    if( read(fd, &header->no_of_sections, 1) != 1){ // citim si verificam numarul de sectiuni
        printf("ERROR\neroare la citirea din fisier -- no of sections");
        free(header);
        close(fd);
        return NULL;
    }
    if(header->no_of_sections < 4 || header->no_of_sections > 10){
        printf("ERROR\nwrong sect_nr");
        free(header);
        close(fd);
        return NULL;
    }

    header->sections = (struct section*) malloc (header->no_of_sections * sizeof(struct section)); // alocam spatiu pentru array-ul  structuri section in care tinem date despre sectiuni

    for(int i = 0; i < header->no_of_sections; i++){ // pentru fiecare sectiune
        if(read(fd, &header->sections[i].sect_name, sizeof(header->sections[i].sect_name)-1) != sizeof(header->sections[i].sect_name)-1){ //citim numele
            printf("ERROR\neroare la citirea din fisier -- section name");
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if( read(fd, &header->sections[i].sect_type, sizeof(header->sections[i].sect_type)) != 1){ //citim si verificam tipul
            printf("ERROR\neroare la citirea din fisier -- section type");
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }
        if(header->sections[i].sect_type != 39 && header->sections[i].sect_type != 86){
            printf("ERROR\nwrong sect_types");
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if(read(fd, &header->sections[i].sect_offset, sizeof(header->sections[i].sect_offset)) != sizeof(header->sections[i].sect_offset)){ //citim offsetul
            printf("ERROR\neroare la citirea din fisier -- section offset");
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if(read(fd, &header->sections[i].sect_size, sizeof(header->sections[i].sect_size)) != sizeof(header->sections[i].sect_size)){ // citim dimensiunea
            printf("ERROR\neroare la citirea din fisier -- section size");
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }
    }
    
    close(fd);
    return header;
}



void print_parse(char * path){
    struct Header* header = parse(path);
    if(header != NULL){ // afisam datele necesare pentru 2.4 daca structura header s-a creat cu succes
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", header->version, header->no_of_sections);
        for(int i = 0; i < header->no_of_sections; i++){
            header->sections[i].sect_name[8] = 0;
            printf("section%d: %s %d %d\n", i+1, header->sections[i].sect_name, header->sections[i].sect_type, header->sections[i].sect_size);
        }
        free(header->sections); // dezalocam memoria
        free(header);
    }
    return;
}



int extract(char* path, int sectionNr, int line){
    int fd = open(path, O_RDONLY);  //deschidem fisierul
    if(-1 == fd){
        printf("ERROR\ninvalid file");
        return -1;
    }
    if(line < 1 ){ // verificam ca nr liniei sa fie valid
        printf("ERROR\ninvalid line");
        close(fd);
        return -1;
    }
    struct Header* header = parse(path); //aflam date despre fisier apeland functia parse
    if(header == NULL){
        printf("ERROR\ninvalid file");
        close(fd);
        free(header);
        return -1;
    }

    if(sectionNr < 1 || sectionNr > header->no_of_sections){ //verificam numarul sectiunii sa fie valid
        printf("ERROR\ninvalid section");
        free(header->sections);
        close(fd);
        free(header);
        return -1;
    }

    lseek(fd, header->sections[sectionNr-1].sect_offset, SEEK_SET); //ne pozitionam la inceputul sectiunii
    char* buff = (char*) malloc (sizeof(char) * header->sections[sectionNr-1].sect_size + 1);
    buff[header->sections[sectionNr-1].sect_size] = 0;

    if(read(fd, buff, header->sections[sectionNr-1].sect_size) != header->sections[sectionNr-1].sect_size){//citim datele din sectiune
        printf("ERROR\neroare la citirea din fisier");
        free(header->sections);
        free(header);
        free(buff);
        close(fd);
        return -1;
    }

    int i = header->sections[sectionNr-1].sect_size - 1; //incepe sa citim de la sfarsitul buff spre inceput pana cand gasim un nr de (line-1) caractere '\n'-new line 
    while( i >= 0 && line > 1){
        if(buff[i] == '\n'){
            line--;
        }
        i--;
    }
    if(i < 0 || line > 1){ //verificam daca nr de linii dat ca input nu este mami mare decat numarul de linii al sectiunii
        printf("ERROR\ninvalid line");
        free(header->sections);
        free(header);
        free(buff);
        close(fd);
        return -1;
    }
    printf("SUCCESS\n"); //afisam linia de la sfarsit spre inceput caracter cu caracter
    while( i >= 0 && buff[i] != '\n'){
        printf("%c", buff[i]);
        i--;
    }
    printf("\n");

    free(header->sections);
    free(header);
    free(buff);
    close(fd);
    return 0;
}

struct Header * parseWithoutprintfs(char* path){ //functia de parse returneaza pointer la o structura header care contine toate datele din header
    int fd = open(path, O_RDONLY);  //descidem fisierul
    if(-1 == fd){
        return NULL;
    }
    lseek(fd, -1, SEEK_END); //pozitionam cursorul la sfarsit pentru a putea citi magic-ul
    char magic;
    if( read(fd, &magic, 1) != 1){ //citim si verificam magic
        close(fd);
        return NULL;
    }
    if(magic != 'Q'){
        close(fd);
        return NULL;
    }

    struct Header* header = (struct Header*) malloc (sizeof(struct Header) * 1); // alocam spatiu pentru header

    lseek(fd, -3, SEEK_END); //pozitionam cursorul pentru a putea citi header size
    if(read(fd, &header->header_size , 2) != 2){  //citim header size
        free(header);
        close(fd);
        return NULL;
    }

    lseek(fd, -1*header->header_size, SEEK_END); //pozitionam cursorul la inceputul headerului

    if( read(fd, &header->version, 1) != 1){// citim si verificam versiunea
        free(header);
        close(fd);
        return NULL;
    }
    if(header->version < 113 || header->version > 163){
        free(header);
        close(fd);
        return NULL;
    }

    if( read(fd, &header->no_of_sections, 1) != 1){ // citim si verificam numarul de sectiuni
        free(header);
        close(fd);
        return NULL;
    }
    if(header->no_of_sections < 4 || header->no_of_sections > 10){
        free(header);
        close(fd);
        return NULL;
    }

    header->sections = (struct section*) malloc (header->no_of_sections * sizeof(struct section)); // alocam spatiu pentru array-ul  structuri section in care tinem date despre sectiuni

    for(int i = 0; i < header->no_of_sections; i++){ // pentru fiecare sectiune
        if(read(fd, &header->sections[i].sect_name, sizeof(header->sections[i].sect_name)-1) != sizeof(header->sections[i].sect_name)-1){ //citim numele
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if( read(fd, &header->sections[i].sect_type, sizeof(header->sections[i].sect_type)) != 1){ //citim si verificam tipul
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }
        if(header->sections[i].sect_type != 39 && header->sections[i].sect_type != 86){
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if(read(fd, &header->sections[i].sect_offset, sizeof(header->sections[i].sect_offset)) != sizeof(header->sections[i].sect_offset)){ //citim offsetul
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }

        if(read(fd, &header->sections[i].sect_size, sizeof(header->sections[i].sect_size)) != sizeof(header->sections[i].sect_size)){ // citim dimensiunea
            close(fd);
            free(header->sections);
            free(header);
            return NULL;
        }
    }
    
    close(fd);
    return header;
}


int getLineNr(char* buff, unsigned int size){ //afla numarul de linii dintr-un array de caractere
    int lines = 1;
    for(int i = 0; i < size; i++){
        if(buff[i] == '\n'){  //daca am gasit caracterul new line, incrementam lines
            lines++;
        }
    }
    return lines;
}

int findall(char* path, int time){
    
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        printf("ERROR\ninvalid directory path");
        return -1;
    }
    if(time == 0){
        printf("SUCCESS\n");
        time++;
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { //daca numele fisierului este diferit de .(folderul curent) si de ..(folderul parinte)
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name); //cream path-ul complet din pathul directorului in care ne aflam si numele fisierului curent
            if(lstat(fullPath, &statbuf) == 0) { //cream statbuf cu informatii despre fisier
                if(!S_ISDIR(statbuf.st_mode)){
                    
                    struct Header* header = parseWithoutprintfs(fullPath); //luam date despre fisier
                    if(header == NULL){
                        closedir(dir);
                        return -1;
                    }
                    int fd = open(fullPath, O_RDONLY);  //descidem fisierul
                    if(-1 == fd){
                        printf("ERROR\nnu se poate deschide fisierul");
                        free(header);
                        closedir(dir);
                        return -1;
                    }
                    int maxNrLinii = 0;
                    int i = 0;
                    while(i < header->no_of_sections && maxNrLinii < 14){  //cautam cat timp nu am parcurs toate sectiunile si nu am gasit una cu min 14 linii
                        char * buff = (char*) calloc (sizeof(char) , header->sections[i].sect_size + 1);
                        buff[header->sections[i].sect_size] = 0;
                        lseek(fd, header->sections[i].sect_offset, SEEK_SET);
                        if(read(fd, buff, header->sections[i].sect_size) != header->sections[i].sect_size){ //citim o sectiuna curenta in buff
                            printf("ERROR");
                            printf("Eroare la citirea din fisier2");
                            free(header->sections);
                            free(header);
                            free(buff);
                            close(fd);
                            closedir(dir);
                            return -1;
                        }
                        maxNrLinii = getLineNr(buff, header->sections[i].sect_size); //aflam numarul de linii pe care il contine
                        i++;
                        free(buff);
                    }
                    if(maxNrLinii >= 14){ //daca numarul maxim de linii gasite intr-o sectiune este min 14
                        printf("%s\n", fullPath);
                    }
                    
                    close(fd);
                    free(header->sections);
                    free(header);
                    
                } else {
                    if(S_ISDIR(statbuf.st_mode)) { //apelam recursiv daca fisierul curent este un director
                        time++;
                        findall(fullPath, time);
                    }
                }
            }
        }
    }
    closedir(dir);
    return 0;
}



int main(int argc, char **argv){
    char* path = NULL;
    char * function;
    int sectionNr=0;
    int lineNr=0;
    if(argc >= 2){

        int recursive = 0;
        char* cond = NULL;  //stringul din argument care reprezinta filtrul
        int condType = 0;
        for(int i = 1; i < argc; i++){
            char * arg = strtok(argv[i], "=");
            if(strcmp(arg, "recursive") == 0){ //verificam daca argumentul este cuvantul recursive
                recursive = 1;
            } else {
                if(strcmp(arg, "path") == 0){ //verificam daca argumentul ne da un path si il stocam 
                    path = strtok(0, " ");
                } else {
                    if(strcmp(arg, "name_starts_with") == 0){ //verificam daca argumentul este filtrul name_starts_with si il stocam
                        condType = 1;
                        cond = strtok(0, " ");
                    } else {
                        if(strcmp(arg, "permissions") == 0){ //verificam daca argumentul este filtrul permissions si il stocam
                            condType = 2;
                            cond = strtok(0, " ");
                        } else {
                            if(strcmp(arg, "section") == 0){ //verificam daca argumentul este numarul sectiunii
                                sectionNr = atoi(strtok(0, " "));
                            } else {
                                if(strcmp(arg, "line") == 0){ //verificam daca argumentul este numarul liniei
                                    lineNr = atoi(strtok(0, " "));
                                } else {
                                    function = arg;
                                }
                            }
                        }
                    }
                }
            }
        }


        if(strcmp(function, "variant") == 0){  //in functie de stringul citit ca functie, apelam functia corespunzatoare
            printf("10917\n");
            return 0;
        } else {
            if(strcmp(function, "list") == 0){
                list(path, recursive, condType, cond);
                return 0;
            } else {
                if(strcmp(function, "parse") == 0){
                    print_parse(path);
                    return 0;
                } else {
                    if(strcmp(function, "extract") == 0){
                        extract(path, sectionNr, lineNr);
                        return 0;
                    } else {
                        if(strcmp(function, "findall") == 0){
                            findall(path, 0);
                            return 0;
                        }
                    }
                }
            }
        }


    }
    return 0;
}