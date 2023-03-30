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

#define MAX_PATH 512
int listLiniar(char * path, int condType, char* cond){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char filePath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    printf("SUCCESS");
    // read one-by-one dir entries until NULL returned
    while((entry = readdir(dir)) != NULL) {
        // avoid "." and ".." as they are not useful
        if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            //printf("%s\n", entry->d_name);
            // build the complete path = dirpath + direntry’s name
            snprintf(filePath, MAX_PATH, "%s/%s", path, entry->d_name);

            if(lstat(filePath, &statbuf) == 0) {

                if(condType == 0){
                    printf("\n%s", filePath);
                }
                if(condType==1 ){
                    int sem = 1;
                        int i = 0;
                        while(i<256 && cond[i] != 0 && entry->d_name[i] != 0 && sem){
                            if(cond[i] != entry->d_name[i]){
                                sem = 0;
                            }
                            i++;
                        }
                        if (entry->d_name[i-1] == 0 && cond[i-1] != 0)
                        {
                            sem = 0;
                        }
                        
                        if(sem){
                            printf("\n%s", filePath);
                        }
                }
                if(condType==2){
                    int mode = 0;
                    for(int i=0; i<9; i++){
                        mode *=2;
                        if(cond[i] == 'r' || cond[i] == 'w' || cond[i] == 'x'){
                            mode ++;
                        } else {
                            if(cond[i] != '-'){
                                perror("ERROR\nCaractere invalide la permisiuni");
                            }
                        }
                    }
                    if((statbuf.st_mode & 0777) == mode){
                        printf("\n%s", filePath);
                    }
                }
            }
            
        }
    }   
    closedir(dir);
    return 0;
}

int listRecursive(char* path, int condType, char* cond){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        perror("ERROR \ninvalid directory path");
        return -1;
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
                if(condType == 0){
                    printf("\n%s", fullPath);
                }
                if(condType==1 ){
                    int sem = 1;
                    int i = 0;
                    while(i<256 && cond[i] != 0 && entry->d_name[i] != 0 && sem){
                        if(cond[i] != entry->d_name[i]){
                            sem = 0;
                        }
                        i++;
                    }
                    if (entry->d_name[i-1] == 0 && cond[i-1] != 0)
                    {
                        sem = 0;
                    }
                    
                    if(sem){
                        printf("\n%s", fullPath);
                    }
                }
                if(condType==2){
                    int mode = 0;
                    for(int i=0; i<9; i++){
                        mode *=2;
                        if(cond[i] == 'r' || cond[i] == 'w' || cond[i] == 'x'){
                            mode ++;
                        } else {
                            if(cond[i] != '-'){
                                perror("ERROR\nCaractere invalide la permisiuni");
                            }
                        }
                    }
                }
                
                if(S_ISDIR(statbuf.st_mode)) {
                    listRecursive(fullPath, condType, cond);
                }
            }
        }
    }
    closedir(dir);
    return 0;
}

void list(char* path, int recursive, int condType, char* cond){
    if(recursive == 1){
        printf("SUCCESS");
        listRecursive(path, condType, cond);
        printf("\n");
    } else {
        listLiniar(path, condType, cond);
    }
}

int parse(char* path){
    int fd = open(path, O_RDONLY);
    if(-1 == fd){
        perror("nu se poate deschide fisierul");
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    lseek(fd, -1, SEEK_END);
    char magic;
    if( read(fd, &magic, 1) != 1){
        perror("ERROR\neroare la citirea din fisier -- magic");
        close(fd);
        return -1;
    }
    if(magic != 'Q'){
        perror("ERROR\nwrong magic");
        close(fd);
        return -1;
    }

    short headerSize;
    lseek(fd, -3, SEEK_END);
    if(read(fd, &headerSize, 2) != 2){
        perror("ERROR\neroare la citirea din fisier -- header size");
        close(fd);
        return -1;
    }
    lseek(fd, -1*headerSize, SEEK_END);

    unsigned char version = 0;
    if( read(fd, &version, 1) != 1){
        perror("ERROR\neroare la citirea din fisier -- version");
        close(fd);
        return -1;
    }
    if(version < 113 || version > 163){
        perror("ERROR\nwrong version");
        close(fd);
        return -1;
    }

    unsigned char sectionsNo;
    if( read(fd, &sectionsNo, 1) != 1){
        perror("ERROR\neroare la citirea din fisier -- no of sections");
        close(fd);
        return -1;
    }
    if(sectionsNo < 4 || sectionsNo > 10){
        perror("ERROR\nwrong sect_nr");
        close(fd);
        return -1;
    }

    struct section* sections = (struct section*) malloc (sectionsNo * sizeof(struct section));

    


    for(int i = 0; i < sectionsNo; i++){
        if(read(fd, &sections[i].sect_name, sizeof(sections[i].sect_name)-1) != sizeof(sections[i].sect_name)-1){
            perror("ERROR\neroare la citirea din fisier -- section name");
            close(fd);
            free(sections);
            return -1;
        }
        if( read(fd, &sections[i].sect_type, sizeof(sections[i].sect_type)) != 1){
            perror("ERROR\neroare la citirea din fisier -- section type");
            close(fd);
            free(sections);
            return -1;
        }
        if(sections[i].sect_type != 39 && sections[i].sect_type != 86){
            perror("ERROR\nwrong sect_types");
            close(fd);
            free(sections);
            return -1;
        }

        if(read(fd, &sections[i].sect_offset, sizeof(sections[i].sect_offset)) != sizeof(sections[i].sect_offset)){
            perror("ERROR\neroare la citirea din fisier -- section offset");
            close(fd);
            free(sections);
            return -1;
        }
        if(read(fd, &sections[i].sect_size, sizeof(sections[i].sect_size)) != sizeof(sections[i].sect_size)){
            perror("ERROR\neroare la citirea din fisier -- section size");
            close(fd);
            free(sections);
            return -1;
        }
    }

    printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, sectionsNo);
    for(int i = 0; i < sectionsNo; i++){
        sections[i].sect_name[8] = 0;
        printf("section%d: %s %d %d\n", i+1, sections[i].sect_name, sections[i].sect_type, sections[i].sect_size);
    }
    
    free(sections);
    close(fd);
    return 0;
}

int main(int argc, char **argv){
    char* path = NULL;
    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("10917\n");
            return 0;
        } 

        if(strcmp(argv[1], "list" ) == 0){
            int recursive = 0;
            char* cond = NULL;
            int condType = 0;
            for(int i = 2; i < argc; i++){
                if(strcmp(argv[i], "recursive") == 0){
                    recursive = 1;
                } else {

            char * arg = strtok(argv[i], "=");
                    if(strcmp(arg, "path") == 0){
                        path = strtok(0, " ");
                    } else {
                        if(strcmp(arg, "name_starts_with") == 0){
                            condType = 1;
                            cond = strtok(0, " ");
                        } else {
                            if(strcmp(arg, "permissions") == 0){
                                condType = 2;
                                cond = strtok(0, " ");
                            }
                        }
                    }
                }
            }
            list(path, recursive, condType, cond);
        } 
        char * function;
        for(int i = 0; i < argc; i++){
            if(strcmp(strtok(argv[i],"="), "path") == 0){
                path = strtok(0, " ");
            } else {
                function = argv[i];
            }
        }
        if(strcmp(function, "parse") == 0){
            parse(path);
        }


    }
    return 0;
}