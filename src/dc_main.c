/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "dc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    unsigned buffersize = 1024;
    char *buffer = malloc(buffersize);
    int c;
    
    struct DC_Context *const ctx = DC_CreateContext();
    
    (void)argc;
    (void)argv;
    if(ctx == NULL){
        fputs("Could not create calculation context.\n", stderr);
        return EXIT_FAILURE;
    }
    
    do{
        unsigned at = 0;
        while((c = getchar()) && c != EOF && c != '\n'){
            buffer[at++] = c;
            if(at + 2 == buffersize){
                char *newbuffer = realloc(buffer, buffersize <<= 1);
                if(newbuffer == NULL){
                    free(buffer);
                    fputs("Out of memory.\n", stderr);
                    return EXIT_FAILURE;
                }
            }
        }
        buffer[at] = 0;
        
        {
            struct DC_Calculation *const calc =
                DC_Compile(ctx, buffer, 0, NULL);
            if(calc == NULL){
                fputs("Could not create calculation.\n", stderr);
                continue;
            }
            else{
                const char *const err = DC_GetError(calc);
                if(err != NULL){
                    fprintf(stderr, "Calculation error: %s\n", err);
                }
                else{
                    const float result = DC_Calculate(calc, NULL);
                    printf("%s = %f\n", buffer, result);
                }
                DC_Free(ctx, calc);
            }
        }
    }while(c != EOF);

    free(buffer);
    DC_FreeContext(ctx);
    return EXIT_SUCCESS;
}
