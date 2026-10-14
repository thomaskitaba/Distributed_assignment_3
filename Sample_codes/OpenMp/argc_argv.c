#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]){


    if (argc > 1) {
        printf("========================\n");
        for (int i = 1; i < argc; i++) {
            printf("argument %d = %s\n", i, argv[i]);
            printf("sizeof argv[%d] = %ld\n", i, strlen(argv[i]));
            for (int j = 0; j < strlen(argv[i]);j++ )
            {
                
                printf("---- char %d = %c\n", j, argv[i][j]);
            }
        }

        printf("========================");
    }
    else {
        printf("No arguments provided\n");
    }
    return 0;
}