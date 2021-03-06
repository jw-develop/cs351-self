#include "cachelab.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// function prototypes
void printHelp();
FILE* openTrace(char* filePath); 

const int WORD_SIZE = 64;
// a struct to represent a block
typedef struct block {
    // is this a valid block?
    char valid;
    // the block's tag
    int tag;
    // the last iteration where this was used
    int lastUsed;
} block;



int main(int argc, char** argv)
{
    // should we have verbose output
    char verbose = 0;
    // should we show the help message and exit
    char help = 0;

    /*
     * one letter variable name where chosen for consistency 
     * with the book and the lab assigment
     */

    // the number of set index bits 
    int s = -1;
    // the number of lines per set
    int E = -1;
    // the number of block bits
    int b = -1;
    // the name of the tracefile to examine
    char* tracePath = NULL;
    // the file pointer to the trace 
    FILE* trace;
    int arg;
    for (arg = 1; arg < argc; arg++) {
        assert(argv[arg][0] == '-');
        switch (argv[arg][1]) {
            case 'h':
                help = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(argv[++arg]);
                break;
            case 'E':
                E = atoi(argv[++arg]);
                break;
            case 'b':
                b = atoi(argv[++arg]);
                break;
            case 't':
                tracePath = argv[++arg];
                break;
            default:
                help = 1;
        }
    }
    // does the program have all the args needed to run
    // and do those arguements have non-negative values
    char canRun = s >= 0 && E >= 0 && b >= 0 && tracePath > 0;
    if (canRun){
        // open the trace file 
        trace = openTrace(tracePath);
        if (trace == NULL)
            canRun = 0;
    }

    // if it can't run display an error and the help dialogue
    if (!canRun) {
        printf("./csim: Missing required command line argument\n");
        help = 1;
    }

    // prints a set help dialogue
    if (help) {
        printHelp();
    }
    // run if the user didn't need help
    else {
        assert(canRun);
        int hits = 0;
        int misses = 0;
        int evictions = 0;
        // create the "cache"
        int cacheSize = (1<<s) * E;
        //int numSets = 1<<s;
        block* cache = malloc(cacheSize*sizeof(block));
        int i;
        for (i = 0; i < cacheSize; i++) {
            cache[i].valid = 0; 
        }
        // iterate through the lines in the file
        char type;
        int addr;
        int size;
        int counter = 0;
        while (fscanf(trace, " %c %x,%d\n", &type, &addr, &size) != EOF) {
            int numAccesses;
            switch (type) {
                case 'S':
                case 'L':
                    numAccesses = 1;
                    if (verbose) {printf("\n%c %x,%d ", type, addr, size);}
                    break;
                case 'M':
                    numAccesses = 2;
                    if (verbose) {printf("\n%c %x,%d ", type, addr, size);}
                    break;
                case 'I':
                    numAccesses = 0;
                    break;
                default:
                    fprintf(stderr, "unknown symbol: %c at start of string\n", type);
                    assert(0);
            }
            while(numAccesses > 0) {
                counter++;
                // the set this address belongs in 
                int set = (addr>>b) & ((1<<s) - 1);
                
                // the tag for this address
                int tag = (addr>>(b+s)) & ((1<<(WORD_SIZE - (b+s))) - 1);

                //printf("address: %x, set: %x, tag %x", addr, set, tag);
                // search the set for the tag

                // did we find the block
                char found = 0;
                // if we didn't which block should we replace it with
                int replaceIndex = -1;
                int cacheIndex;
                for (cacheIndex = set*E; cacheIndex < ((set+1)*(E)); cacheIndex++) {
                    assert(cacheIndex < cacheSize);
                    if (cache[cacheIndex].valid && tag == cache[cacheIndex].tag) {
                        found = 1;
                        break;
                    }
                    if (replaceIndex == -1 || cache[cacheIndex].valid == 0 
                            || cache[cacheIndex].lastUsed < cache[replaceIndex].lastUsed) {
                        replaceIndex = cacheIndex;
                    }
                }
                assert(found || replaceIndex != -1);
                if (found) {
                    assert(cache[cacheIndex].tag == tag);
                    if (verbose) {printf(" hit (s: %x t: %x)",  set, tag );}
                    hits++;
                    cache[cacheIndex].lastUsed = counter;
                } 
                else {
                    misses++;
                    if (verbose) {printf(" miss (s: %x t: %x)", set, tag);}
                    if (cache[replaceIndex].valid == 1) {
                        evictions++;
                        if (verbose) {printf(" eviction (s: %x t: %x)",set
                                , cache[replaceIndex].tag);}
                    }
                    cache[replaceIndex].tag = tag;
                    cache[replaceIndex].valid = 1;
                    cache[replaceIndex].lastUsed = counter;
                }
                numAccesses--;
            }



        }
       if (verbose) {printf("\n");}
       assert(fclose(trace) == 0);
       free(cache);
       printSummary(hits, misses, evictions);
    }


    return 0;
}


/*
 * printHelp - prints the preset help dialogue
 */
void printHelp(){
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");

    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");

    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

/*
 * openTrace - opens the trace file and handles input errors
 */
FILE* openTrace(char* filePath) {
    errno = 0;
    FILE* file;
    assert(filePath != NULL);
    file = fopen(filePath, "r");
    if (file == NULL) {
        fprintf(stderr, "failed to open trace file: %s\n", strerror(errno));
    }
    return file;
}
