#include "cachelab.h"
#include<stdio.h>
#include <stdbool.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<tgmath.h>

#include "tools.h"

#define FILENAME_SIZE_LIMIT 80
#define DEBUG true

#define LOAD 1
#define STORE 2
#define MODIFY 3

#define HELP_FLAG "-h"
#define VERBOSE_FLAG "-v" 
#define SET_FLAG "-s"
#define BLOCK_FLAG "-b"
#define ASSOC_FLAG "-E"
#define TRACE_FLAG "-t"
/** 
 * Taylor King
 * kingtb
 */

typedef struct cache { 
  int associativity;
  int block_size;
  int block_bits;
  int set_size;
  int set_bits;
  int ** tags;
      
} cache_t;

typedef struct trace_line {
  int operation;
  unsigned int address;
  int size;
} trace_line_t;

typedef struct result {
  int num_hits;
  int num_misses;
  int num_evictions;
} result_t;

// function prototypes
char * help_message();
int get_operation(char op_char);
trace_line_t process_line(char * line_text);
result_t read_to_cache(cache_t cache, char * fileName);
result_t run_simulation(trace_line_t * lines, int num_lines, cache_t cache);
result_t simulate_line(trace_line_t line, cache_t settings, int ** data);
int main(int argc, char * argv[])
{
    // flags to make sure we have everything.
    bool s_given = false;
    bool trace_given = false;
    bool assoc_given = false;
    bool block_given = false;

    bool verbose = false;

    char * file_name = malloc(80);
    cache_t cache;

    for (int i = 0; i < argc; i++) {
      // check to see if they asked for the help
      if (strcmp(argv[i], HELP_FLAG) == 0) {
        printf(help_message());
        return 0;
      }
      // check to see if they set it to verbose mode
      if (strcmp(argv[i], VERBOSE_FLAG) == 0) {
        // they used the verbose flag
        verbose = true;
      }


      // get the size of the set
      if (strcmp(argv[i], SET_FLAG) == 0) {
        char * value = argv[i+1];
        cache.set_bits = atoi(value);
        cache.set_size = pow(2, atoi(value));
        s_given = true; 
      }

      // get the size of the 
      if (strcmp(argv[i], ASSOC_FLAG) == 0) {
        char * value = argv[i+1];
        cache.associativity = atoi(value);
        assoc_given = true;
      }
      if (strcmp(argv[i], BLOCK_FLAG) == 0) {
        char * value = argv[i+1];
        cache.block_bits = atoi(value);
        cache.block_size = pow(2, atoi(value));
        block_given = true;
      }
      if (strcmp(argv[i], TRACE_FLAG) == 0) {
        char * value = argv[i+1];
        strcpy(file_name, value);
        trace_given = true;
      }
    }

    // if they didn't give all the required arguments we can't do anything.
    if (!s_given || !trace_given || !assoc_given || !block_given) {
       printf("Incorrect arguments supplied\nusage: ./csim -s 4 -E 1 -b 3 -t filename.trace\n");
       return 1;
    }
    // pass to printSummary the number of hits, misses and evictions
    result_t run_result = read_to_cache(cache, file_name);
    printSummary(run_result.num_hits, run_result.num_misses, run_result.num_evictions);
    return 0;
}


result_t read_to_cache(cache_t cache, char * file_name) {
  int num_lines = 0; 
  FILE * trace_file ;
  char line_text[80];
  trace_file = fopen(file_name, "r");
  // count the number of lines so we can store them in an array based on their line number  
  while (fgets(line_text, 80, trace_file)) {
    num_lines++;
  }
  // reset the file 
  trace_file = fopen(file_name, "r");
  trace_line_t lines[num_lines];
  int line_num = 0;
  // get the actual line text
  while (fgets(line_text, 80, trace_file)) {
    //extract the struct from the line
    lines[line_num] = process_line(line_text);
    line_num++;
  }
  return run_simulation(lines, num_lines, cache);
}


result_t run_simulation(trace_line_t * lines, int num_lines, cache_t cache) {
  cache.tags = malloc(sizeof(int*) * cache.set_size);
  // initialize the cache
  result_t result; result.num_hits = 0; result.num_misses = 0; result.num_evictions = 0;
  for(int i = 0; i < cache.set_size; i++) {
    cache.tags[i] = malloc(sizeof(int) * cache.associativity);  
    for(int j = 0; j < cache.associativity; j++) {
      cache.tags[i][j] = -1;
    }
  }
  for(int q = 0; q < num_lines; q++) {
    trace_line_t line = lines[q];
    if(line.operation != LOAD && line.operation != STORE && line.operation != MODIFY) {
      continue; // if the operation is not valid, we wont process this line 
    }
    uint64_t set_bits = get_bits(line.address, cache.block_bits, cache.block_bits + cache.set_bits - 1);
    uint64_t tag_bits = get_bits(line.address, cache.block_bits + cache.set_bits, 63);
    int * tags = cache.tags[set_bits];
    bool found = false;
    // check and see if we can find the tag in the cache
    for(int i = 0; i < cache.associativity; i++) {
      if(tags[i] == tag_bits) {
        if(line.operation == MODIFY) {
          result.num_hits+=2;
        } else {
          result.num_hits++;
        }
        found = true;
        // if we found it, move the tag to the front
        for(int j = i; j > 0; j--) {
          tags[j] = tags[j-1];
        }
        tags[0] = tag_bits;
      }
    }
    // we didnt find it
    if(!found) {
      // if were doing a modify. were gonna be accessing the block twice
      if(line.operation == MODIFY) {
        result.num_misses++;
        result.num_hits++;
      } else {
        result.num_misses++;
      }
      // if there is content on the last block in the set, we're evicting somebody
      if(tags[cache.associativity - 1] > -1) {
        result.num_evictions++;
      }
      // shift everyting down to make the eviction
      for(int i = cache.associativity - 1; i > 0; i--) {
        tags[i] = tags[i-1];
      }
      // set the first cell to our tag
      tags[0] = tag_bits;
    }
  }
  return result;
}

trace_line_t process_line(char * line_text) {
  char op;
  trace_line_t result;
  sscanf(line_text, " %c %x,%d", &op, &result.address, &result.size);
  result.operation = get_operation(op);
  return result;
}

int get_operation(char op_char) {
  switch(op_char) {
    case 'L':
      return LOAD;
    case 'S':
      return STORE;
    case 'M':
      return MODIFY;
  }
  return -1;
}

char * help_message() {
  return "usage: ./csim -s 4 -E 1 -b 3 -t filename.trace\n\nTaylor King Cache Simulator\n-h\tprint this help message\n-v\toptionally set verbose flag that displays trace info\n-s\t<s> number of set index bits (S=2^s number of sets)\n-E\t<E>Associativity (number of lines per set)\n-b\t<b> number of block bits (b=2^b the block size)\n-t\t<tracefile> name of the valgrind trace to replay.\n\n";
}

