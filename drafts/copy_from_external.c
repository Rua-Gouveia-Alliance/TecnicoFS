#include<stdio.h>
#include<stdlib.h>


int main(void) {
    const char* source_path = "test1.in";
    const char* dest_path = "test1.out";
    FILE* source_handle = fopen(source_path, "r");
    if (source_handle == NULL) {
        printf("Source error\n");
        return -1;
    }

    FILE* dest_handle = fopen(dest_path, "w");
    if (dest_handle == NULL) {
        printf("Destination error\n");
        fclose(source_handle);
        return -1;
    }

    size_t block_size = 1024;
    size_t read_result;
    char *buffer[block_size];
    
    while(!feof(source_handle)) {
        read_result = fread(buffer, sizeof(char), block_size, source_handle);
        if (ferror(source_handle) || fwrite(buffer, sizeof(char), read_result, dest_handle) != read_result) {
            printf("Writing error\n");
            fclose(source_handle);
            fclose(dest_handle);
            return -1;
        }
    }

    printf("Success\n");
    fclose(source_handle);
    fclose(dest_handle);
    return 0;
}