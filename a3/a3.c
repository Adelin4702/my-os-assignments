#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define WRITE_FIFO "RESP_PIPE_10917"
#define READ_FIFO "REQ_PIPE_10917"
#define SH_MEM_NAME "/Xjxox2"

char* map = NULL;
volatile char *sharedMem = NULL;
unsigned int MemSize, fileSize;

int creareaRegiuniiMemPartajata(unsigned int size){
    int shmFd;
    shmFd = shm_open(SH_MEM_NAME, O_CREAT | O_RDWR, 0644);
    if(shmFd < 0){
        return 1;
    }
    ftruncate(shmFd, size);
    sharedMem = (volatile char *)mmap(0, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    MemSize = size;
    return 0;
}

int scriereMemPartajata(unsigned int offset, unsigned int value){
    if(offset < 0 || offset > MemSize - sizeof(unsigned int)){
        return 1;
    }   

    for(int i = offset; i < offset + sizeof(unsigned int); i++){
        sharedMem[i] = value % (16 * 16);
        value = value / (16 * 16);
    }
    return 0;
}

int mapareFisier(char * fileName){
    int fFisier;
    fFisier = open(fileName, O_RDONLY);
    if(fFisier == -1){
        printf("Fisierul nu exista");
        return 1;
    }
    fileSize = lseek(fFisier, 0, SEEK_END);
    lseek(fFisier, 0, SEEK_SET);

    map = (char*)mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fFisier, 0);
    if(map == (void*)-1) {
        printf("Could not map file");
        close(fFisier);
        return 1;
    }
    close(fFisier);
    return 0;
}

int citireDinFisierOffset(unsigned int offset, unsigned int nr){
    if(offset < 0 || (offset + nr) > fileSize || sharedMem == NULL || map == NULL){
        return 1;
    }
    for(int i = 0; i < nr; i++){
        sharedMem[i] = map[offset + i];
    }
    return 0;
}

int citireDinSectiune(unsigned int section_nr, unsigned int offset, unsigned int bytesNr){
    if(map == NULL || sharedMem == NULL){
        return 1;
    }
    
    unsigned int hSize = ((map[fileSize - 2] * 256) & 0xff00) +  (map[fileSize - 3] & 0x00ff);

    if(section_nr > ((hSize - 5) / 17)){
        return 1;
    }

    unsigned int index = fileSize - hSize; // inceputul headerului
    index += 2; // prima sectiune

    for(int i = 1; i < section_nr; i++){
        index += 17;
    }

    index += 9; // offsetul ei


    unsigned int sectOffset = ((((unsigned char)(map[index + 3]))* 256 * 256 * 256) & 0xff000000) + ((map[index + 2] * 256 * 256) & 0x00ff0000) +
                            ((map[index + 1] * 256) & 0x0000ff00) + (map[index] & 0x000000ff);
    unsigned int sSize = map[index + 7] * 256 * 256 * 256 + map[index + 6] * 256 * 256 +
                          map[index + 5] * 256 + map[index + 4];
                    
    unsigned int readingOffset = sectOffset + offset;

    if(readingOffset - sectOffset > sSize){
        return 1;
    }

    for(int i = 0; i < bytesNr; i++){
        sharedMem[i] = map[readingOffset + i];
    }

    return 0;
}

int citesteDinSpatiulLogic(int logicalOffset, int bytesNr){
    if(map == NULL || sharedMem == NULL){
        return 1;
    }

    unsigned int hSize = ((map[fileSize - 2] * 256) & 0xff00) +  (map[fileSize - 3] & 0x00ff);
    unsigned int index  = fileSize - hSize + 1;
    unsigned char nrSections = map[index++];

    index = fileSize - hSize + 2;
    unsigned int size = 0, sectOffset = 0;
    for(int i = 0; i < nrSections; i++){
        unsigned int sectSize = ((((unsigned char)(map[index + 3 + 13]))* 256 * 256 * 256) & 0xff000000) + ((map[index + 2 + 13] * 256 * 256) & 0x00ff0000) +
                            ((map[index + 1 + 13] * 256) & 0x0000ff00) + (map[index + 13] & 0x000000ff);

        if(logicalOffset >= size && logicalOffset < ((size + sectSize) / 4096 + 1)*4096 ){
            sectOffset = ((((unsigned char)(map[index + 3 + 9]))* 256 * 256 * 256) & 0xff000000) + ((map[index + 2 + 9] * 256 * 256) & 0x00ff0000) +
                        ((map[index + 1 + 9] * 256) & 0x0000ff00) + (map[index + 9] & 0x000000ff);
            break;
        } 
        else 
        {   
            if(sectSize % 4096 == 0){
                size += sectSize;
            } else {
                size = ((size + sectSize) / 4096 + 1) * 4096;
            }
            index += 17;
        }
    }
    if(sectOffset == 0){
        return 1;
    }

    sectOffset += logicalOffset - size;
    for(int i = 0; i < bytesNr; i++){
        sharedMem[i] = map[sectOffset + i];
    }
    return 0;
    
}

int main(int argc, char** argv)
{
    int fRead = -1, fWrite = -1;

    if(mkfifo(WRITE_FIFO, 0600) != 0){
        printf("Error creating FIFO");
        return 1;
    }

    fRead = open(READ_FIFO, O_RDONLY);
    if(fRead == -1){
        printf("Could not open READ_FIFO\n");
        unlink(WRITE_FIFO);
        return 1;
    }

    fWrite = open(WRITE_FIFO, O_WRONLY);
    if(fWrite == -1){
        printf( "Could not open WRITE_FIFO\n");
        unlink(WRITE_FIFO);
        return 1;
    }


    char * hello = "HELLO#";
    for(int i = 0; i < strlen(hello); i++){
        write(fWrite, &hello[i], sizeof(hello[i]));
    }

    while( 1 ){
        char input[250];
        int i = 0;
        while(read(fRead, &input[i], sizeof(char)) > 0 && input[i] != '#'){
            i++;
        }
        input[i + 1] = '\0';
        if(strcmp(input, "VARIANT#") == 0){
            char * s = "VALUE#";
            for(int i = 0; i < strlen(input); i++){
                write(fWrite, &input[i], sizeof(char));
            }
            unsigned int variant = 10917;
            write(fWrite, &variant, sizeof(unsigned int));
            for(int i = 0; i < strlen(s); i++){
                write(fWrite, &s[i], sizeof(char));
            }
        } 
        else 
        {   
            if(strcmp(input, "EXIT#") == 0){
                shm_unlink(SH_MEM_NAME);
                munmap(map, fileSize);
                close(fWrite);
                unlink(WRITE_FIFO);
                return 0;
            } 
            else 
            {
                if(strcmp(input, "CREATE_SHM#") == 0){
                    unsigned int size;
                    read(fRead, &size, sizeof(unsigned int));

                    for(int i = 0; i < strlen(input); i++){
                        write(fWrite, &input[i], sizeof(char));
                    }

                    char * s = "ERROR#";
                    if(creareaRegiuniiMemPartajata(size) == 0){
                        s = "SUCCESS#";
                    }

                    for(int i = 0; i < strlen(s); i++){
                        write(fWrite, &s[i], sizeof(char));
                    }
                } 
                else 
                {
                    if(strcmp(input, "WRITE_TO_SHM#") == 0){
                        unsigned int offset, value;
                        read(fRead, &offset, sizeof(unsigned int));
                        read(fRead, &value, sizeof(unsigned int));

                        for(int i = 0; i < strlen(input); i++){
                            write(fWrite, &input[i], sizeof(char));
                        }

                        char * s = "ERROR#";
                        if(scriereMemPartajata(offset, value) == 0){
                            s = "SUCCESS#";
                        }

                        for(int i = 0; i < strlen(s); i++){
                            write(fWrite, &s[i], sizeof(char));
                        }
                    }
                    else 
                    {
                        if(strcmp(input, "MAP_FILE#") == 0){
                            char fileName[250];
                            int k = 0;
                            while(read(fRead, &fileName[k], sizeof(char)) > 0 && fileName[k] != '#' && k < 250){
                                k++;
                            }
                            fileName[k] = '\0';

                            for(int i = 0; i < strlen(input); i++){
                                write(fWrite, &input[i], sizeof(char));
                            }

                            char * s = "ERROR#";
                            if(mapareFisier(fileName) == 0){
                                s = "SUCCESS#";
                            }

                            for(int i = 0; i < strlen(s); i++){
                                write(fWrite, &s[i], sizeof(char));
                            }
                        }
                        else 
                        {
                            if(strcmp(input, "READ_FROM_FILE_OFFSET#") == 0){

                                unsigned int offset, nrBytes;
                                read(fRead, &offset, sizeof(unsigned int));
                                read(fRead, &nrBytes, sizeof(unsigned int));

                                for(int i = 0; i < strlen(input); i++){
                                    write(fWrite, &input[i], sizeof(char));
                                }

                                char * s = "ERROR#";
                                if(citireDinFisierOffset(offset, nrBytes) == 0){
                                    s = "SUCCESS#";
                                }

                                for(int i = 0; i < strlen(s); i++){
                                    write(fWrite, &s[i], sizeof(char));
                                }
                            }
                            else 
                            {
                                if(strcmp(input, "READ_FROM_FILE_SECTION#") == 0){

                                    unsigned int sectionNr, offset, nrBytes;
                                    read(fRead, &sectionNr, sizeof(unsigned int));
                                    read(fRead, &offset, sizeof(unsigned int));
                                    read(fRead, &nrBytes, sizeof(unsigned int));

                                    for(int i = 0; i < strlen(input); i++){
                                        write(fWrite, &input[i], sizeof(char));
                                    }

                                    char * s = "ERROR#";
                                    if(citireDinSectiune(sectionNr, offset, nrBytes) == 0){
                                        s = "SUCCESS#";
                                    }

                                    for(int i = 0; i < strlen(s); i++){
                                        write(fWrite, &s[i], sizeof(char));
                                    }
                                }
                                else 
                                {
                                    if(strcmp(input, "READ_FROM_LOGICAL_SPACE_OFFSET#") == 0){

                                        unsigned int offset, nrBytes;
                                        read(fRead, &offset, sizeof(unsigned int));
                                        read(fRead, &nrBytes, sizeof(unsigned int));

                                        for(int i = 0; i < strlen(input); i++){
                                            write(fWrite, &input[i], sizeof(char));
                                        }

                                        char * s = "ERROR#";
                                        if(citesteDinSpatiulLogic(offset, nrBytes) == 0){
                                            s = "SUCCESS#";
                                        }

                                        for(int i = 0; i < strlen(s); i++){
                                            write(fWrite, &s[i], sizeof(char));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}