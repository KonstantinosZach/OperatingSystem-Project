#include <headers.h>

int count_lines(char* file_name){

    FILE *file = fopen(file_name, "r");
    if(file == NULL){
        perror("Wrong file name\n");
        exit(EXIT_FAILURE);
    }

    char ch;
    int count = 1;
    while((ch = fgetc(file)) != EOF){
        if(ch == '\n')
            count++;
    }
    fclose(file);
    return count;
}

char* get_line(char*file_name, int pos){

    FILE* file = fopen(file_name, "r");
    if(file == NULL){
        perror("Wrong file name\n");
        exit(EXIT_FAILURE);
    }
    char* line = malloc(TEXT_SZ*sizeof(char));

    int counter = 0;
    while (fgets(line, TEXT_SZ*sizeof(char), file)) {
        if(counter == pos)
            break;
        counter++;
    }
    fclose(file);
    return line;
}