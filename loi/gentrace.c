/* gentrace.c
 * Copyright 2014 The KRD Development Team
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * gentrace.c: generate a coherent RWI trace from a human readable file
 */

/* This code was developed by Madhavan Manivannan <madhavan "at" chalmers dot se>
 * Used with permission
 */ 

/* Reads trace file in text format and generates a tool
 * compatible binary trace file. Inp. trace file format:
 * <addr> <tsc> <size> <coreid>
 */

#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>

#ifndef BUILD_GENTRACE_TOOL
  #error "Please compile the gentrace tool using the provided Makefile" 
#endif

//Defines for RWI Trace
#define RWI_TRACE_MAGIC   (0xffe)
#define SUFFIX ".krd"

#include "loi.h"

int main(int argc, char **argv)
{
    FILE *inp, *out;
    char inputfile[100], outputfile[100], buffer[100], *token;

    if(argc != 3) {
        printf("./gentrace <tracenum> <rwinum>\n");
        exit(1);
    }    

    sprintf(inputfile, "trace_%d", atoi(argv[1]));
    sprintf(outputfile, "rwi_%04d" SUFFIX, atoi(argv[2]));
    
    inp = fopen(inputfile, "r");
    if(inp == NULL) {
        printf("Error opening INFILE\n");
        exit(1);
    }

    out = fopen(outputfile, "w");
    if(out == NULL) {
        printf("Error opening OUTFILE\n");
        exit(1);
    }
    
    // RWI file header needed by KRD tool
    uint32_t format= RWI_TRACE_MAGIC;
    //printf("%d\n", format);
    fwrite(&format, sizeof(uint32_t), 1, out);
    
    while(!feof(inp)) {
        if(fgets(buffer, sizeof(buffer), inp) != NULL) {
            struct krd_id_tsc temp;
            token = strtok(buffer, " ");
            temp.id = (uint64_t) atoi(token);
            token = strtok(NULL, " ");
            temp.tsc = (uint64_t) atoi(token);
            token = strtok(NULL, " ");
            temp.size = (uint32_t) atoi(token);
            token = strtok(NULL, " ");
            temp.core = (uint16_t) atoi(token);
            //printf("%ld %ld %d %d\n", temp.id, temp.tsc, temp.size, temp.core);
            fwrite(&temp, sizeof(struct krd_id_tsc), 1, out);
        }
    }
    
    fclose(inp);
    fclose(out);
    return 0;
}
