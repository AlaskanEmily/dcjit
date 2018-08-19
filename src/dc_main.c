/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "dc.h"

/* Gets us strndup on not-windows. */
#define _BSD_SOURCE

/* Might get us sscanf_s? */
#define __STDC_WANT_LIB_EXT1__ 

#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef __GNUC__

#define DC_STRNDUP stdndup

#else

static char *DC_STRNDUP(const char *s, unsigned long l){
    char *const str = malloc(l + 1);
    unsigned i = 0;
    while(i < l && s[i] != '\0'){
        str[i] = s[i];
        i++;
    }
    str[i] = '\0';
    return str;
}

#endif

#if ( defined __STDC_LIB_EXT1__ ) || ( defined _MSC_VER )
    #define DC_SSCANF sscanf_s
#else
    #define DC_SSCANF sscanf
#endif

static float parse_float(const char *str){
    float f;
    DC_SSCANF(str, "%f", &f);
    return f;
}


int main(int argc, char **argv){
    unsigned buffersize = 1024;
    char *buffer = malloc(buffersize);
    int c;
    
    struct DC_Context *const ctx = DC_CreateContext();
    
    unsigned arg_num;
    const unsigned num_args = argc - 1;
    
    /* store the arg names and values in one large buffer. */
    void **const arg_buffer = malloc((sizeof(void*)+sizeof(float))*num_args);
    char **const argument_names = (void*)arg_buffer;
    float *const argument_values = (void*)(arg_buffer + num_args);
    
    if(ctx == NULL){
        fputs("Could not create calculation context.\n", stderr);
        return EXIT_FAILURE;
    }
    
    /* Parse args, if there are any. */
    for(arg_num = 1; arg_num < (unsigned)argc; arg_num++){
        unsigned i = 0;
        while((c = argv[arg_num][i++]) != '='){
            if(c == '\0'){
                fprintf(stderr,
                    "Invalid argument %i, must be <name>=<float>\n", arg_num);
                return EXIT_FAILURE;
            }
        }
        argument_names[arg_num-1] = DC_STRNDUP(argv[arg_num], i - 1);
        argument_values[arg_num-1] = parse_float(argv[arg_num] + i);
    }
    
    /* Read in expression, execute, repeat */
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
        
        if(at != 0){
            struct DC_Calculation *const calc =
                DC_Compile(ctx, buffer, num_args, argument_names);
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
                    const float result = DC_Calculate(calc, argument_values);
                    printf("%s = %f\n", buffer, result);
                }
                DC_Free(ctx, calc);
            }
        }
    }while(c != EOF);

    for(arg_num = 0; arg_num < num_args; arg_num++)
        free(argument_names[arg_num]);
    
    free(arg_buffer);
    free(buffer);
    DC_FreeContext(ctx);
    return EXIT_SUCCESS;
}
