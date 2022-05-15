
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "memory_subsystem_constants.h"
#include "l2_cache.h"


//number of cache entries (2^15)
#define L2_NUM_CACHE_ENTRIES (1<<15)

int main()
{

  printf("Initializing L2\n");
  l2_initialize();


  uint32_t read_data[WORDS_PER_CACHE_LINE];
  uint32_t write_data[WORDS_PER_CACHE_LINE];
  uint32_t evicted_writeback_data[WORDS_PER_CACHE_LINE];
  uint32_t evicted_writeback_address;

  uint8_t status;

  uint32_t i,j;

  //write to each cache line in the L2 cache using
  //l2_cache_access(). If there's a cache miss (as 
  //there should be initially), call l2_insert_line

  printf("Pass 1: Writing to each entry of empty L2 cache\n");

  // Here, i is the word offset in memory of the beginning of the cache line

  for(i=0; i < (L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); i += WORDS_PER_CACHE_LINE) {

    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      write_data[j] = i+j;

    // the address of the cache line to be written to is i * BYTES_PER_WORD (which is 4)
    l2_cache_access(i<<2, write_data, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
      //call l2_insert_line
      l2_insert_line(i<<2, write_data, &evicted_writeback_address, 
                       evicted_writeback_data, &status);
      if (status & 0x1) { //if writeback line was evicted, indicated by lsb of status = 1
        //something was wrong.
        printf("Error: No cache line to evict and write back in Pass 1\n");
        exit(1);
      }
    }
    else {  //cache hit, which shouldn't happen initially
        printf("Error: No cache line hits should happen in Pass 1\n");
        exit(1);
    }
  }

  printf("Pass 2: Reading from each entry of L2 cache\n");


  //Now, second time through the cache, reading, should be a cache hit every time.
  for(i=0; i < (L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); i += WORDS_PER_CACHE_LINE) {

    l2_cache_access(i<<2, NULL, READ_ENABLE_MASK, read_data, &status);

    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
      printf("Error: Cache miss upon read in Pass 2\n");
      exit(1);
    }

    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      if (read_data[j] != i+j) {
        printf("Error: Data read in Pass 2 does not match data written in Pass 1\n");
        exit(1);
      }

  }

  printf("Pass 3: Replacing each entry of L2 cache with new cache line\n");

  for(i = L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE; 
      i < (2 * L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); 
      i += WORDS_PER_CACHE_LINE) {

    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      write_data[j] = i+j;

    l2_cache_access(i<<2, write_data, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
      //call l2_insert_line
      l2_insert_line(i<<2, write_data, &evicted_writeback_address, 
                       evicted_writeback_data, &status);
      if (status & 0x1) { //if writeback line was evicted, indicated by lsb of status = 1
        //something was wrong.
        printf("Error: No cache line should be evicted and written back in Pass 3\n");
        exit(1);
        }
    }
    else {  //cache hit, which shouldn't happen
        printf("Error: No cache hits should happen in Pass 3\n");
        exit(1);
    }
  }

  printf("Pass 4: Writing to cache lines already resident in L2 cache\n");

  for(i = L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE; 
      i < (2 * L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); 
      i += WORDS_PER_CACHE_LINE) {

    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      write_data[j] = (i+j) << 2;

    l2_cache_access(i<<2, write_data, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
        printf("Error: Cache miss in Pass 4 \n");
        exit(1);
        }
  }

  printf("Pass 5: Reading from cache lines written in Pass 1, should miss every time\n");

  //Now reading using addresses from Pass 1, should be a cache miss every time.
  for(i=0; i < (L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); i += WORDS_PER_CACHE_LINE) {

    l2_cache_access(i<<2, NULL, READ_ENABLE_MASK, read_data, &status);

    if (status & 0x1) { //if cache hit, indicated by lsb of status = 1
      printf("Error: Cache hit upon read in Pass 5\n");
      exit(1);
    }
  }

  printf("Pass 6: Reading from cache lines written in Pass 4\n");

  for(i = L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE; 
      i < (2 * L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); 
      i += WORDS_PER_CACHE_LINE) {

    l2_cache_access(i<<2, NULL, READ_ENABLE_MASK, read_data, &status);


    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
        printf("Error: Cache miss in Pass 6\n");
        exit(1);
    }
    
    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      if (read_data[j] != ((i+j) << 2)) {
        printf("Error: Data read in Pass 6 is different from that written in Pass 4\n");
        printf("Should have read %d but read %d instead\n", ((i+j) << 2), read_data[j]);
        exit(1);
    }
  }

  printf("Pass 7: Repeating pass 1 to write to cache lines that are not in L2 and then,\n");
  printf("        upon each miss, calling l2_insert_line. Each line evicted should be\n");
  printf("        written back\n");

  for(i=0; i < (L2_NUM_CACHE_ENTRIES * WORDS_PER_CACHE_LINE); i += WORDS_PER_CACHE_LINE) {

    for(j=0; j < WORDS_PER_CACHE_LINE; j++) 
      write_data[j] = i+j;

    l2_cache_access(i<<2, write_data, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) { //if cache miss, indicated by lsb of status = 0
      //call l2_insert_line
      l2_insert_line(i<<2, write_data, &evicted_writeback_address, 
                       evicted_writeback_data, &status);
      if (!(status & 0x1)) { //if writeback line was not evicted, indicated by lsb of status = 0
        //something was wrong.
        printf("Error: Each evicted cache line should be written back in Pass 7, but wasn't\n");
        exit(1);
      }
    }
    else {  //cache hit, which shouldn't happen initially
        printf("Error: No cache line hits should happen in Pass 7\n");
        exit(1);
    }
  }

  printf("Passed\n");
}

