

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <math.h>
#include <time.h>

#include "memory_subsystem_constants.h"
#include "l1_cache.h"

// There are 2K = 2^11 cache lines
#define L1_NUM_CACHE_LINES (1 << 11)

int main()
{
  
  l1_initialize();

  //Pass 1: Filling all lines of the cache with data

  uint32_t read_data;
  uint32_t write_data[WORDS_PER_CACHE_LINE];
  uint32_t evicted_writeback_data[WORDS_PER_CACHE_LINE];
  uint32_t evicted_writeback_address;

  uint8_t status;

  uint32_t i,j;

  printf("Pass 1: Writing to each entry of empty L1 cache\n");

  //i is the address of a single word

  for(i=0; i < (L1_NUM_CACHE_LINES * WORDS_PER_CACHE_LINE * 4); i+=4) {

    //writing the value i*2 at address i
    l1_cache_access(i, i<<1, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) { 
      //cache miss, should only happen on the first word of a cache line
      //i should be divisible by 32 (because 8 words/line, 4 bytes/word)

      if (i & 0x1F) { //not divisible by 8
	printf("Error: Cache misses should only occur for the first word of a line in Pass 1\n");
	exit(1);
      }

      //populate the first word of the cache line with i*2, the rest with phony data, and insert the cache line

      write_data[0] = i<<1;

      for(j=1; j<WORDS_PER_CACHE_LINE;j++) {
	write_data[j] = 1000 + j;
      }

      l1_insert_line(i, write_data, &evicted_writeback_address, 
		     evicted_writeback_data, &status);

      if (status & 0x1) { //if writeback line was evicted, indicated by lsb of status = 1
	//something was wrong.
	printf("Error: No cache line to evict and write back in Pass 1\n");
	exit(1);
      }
    }
    else {  //cache hit, which should only happen when i is not divisible by 8
      if (!(i & 0x1F)) { //divisible by 8
	printf("Error: Cache hits should not occur for first word of a line in Pass 1\n");
	exit(1);
      }
    }
  }


  printf("Pass 2: Reading back the values written in Pass 1\n");

  for(i=0; i < (L1_NUM_CACHE_LINES * WORDS_PER_CACHE_LINE * BYTES_PER_WORD); i+=BYTES_PER_WORD) {
    //read the value at address i.
    l1_cache_access(i, ~0x0, READ_ENABLE_MASK, &read_data, &status);
    if (read_data != (i<<1)) {  //didn't read back the right value
      printf("Error: Data read back in Pass 2 didn't match the values written in Pass 1\n");
      exit(1);
    }
    if (!(status & 0x1)) { //cache miss, shouldn't happen
      printf("Error: Cache miss, shouldn't occur in Pass 2\n");
      exit(1);
    }
  }


  printf("Pass 3: Generating reads at random addresses, cache misses\n");
  printf("        should occur only for addresses greater than or equal to 64K = 2^16\n");

  srand(time(NULL));

  uint32_t address;

  for(i=0;i<(1<<24);i++) {      // iterate 2^24 times
    address = rand() & ~0x3; //generate a random address, divisible by 4 (word aligned)
    l1_cache_access(address, ~0x0, READ_ENABLE_MASK, &read_data, &status);

    //cache miss should only happen if address >= 2^16
    if ((!(status & 0x1)) && (address < (1 << 16))) {
	printf("Error: Cache miss on address %x, shouldn't occur in Pass 2\n", address);
	exit(1);
      }

    //cache hit should only happen if address <= 2^16
    if ((status & 0x1) && (address >= (1 << 16))) {
	printf("Error: Cache hit on address %x, shouldn't occur in Pass 2\n", address);
	exit(1);
      }
  }

  printf("Pass 4: Testing cache replacement policy.\n");

  l1_initialize();  //need to start with a clean cache

  //for each of the 512 sets in the L1 cache

  uint32_t setnum;

#define LINES_PER_SET 4
#define SETS_PER_CACHE 512

#define L1_SET_INDEX_SHIFT 5
#define L1_ADDRESS_TAG_SHIFT 14  

  for (setnum=0; setnum<SETS_PER_CACHE;setnum++) {

  //insert 4 lines into each set, calling their addresses r0d0,r0d1,r1d0,r1d1.
  //Each address is constructed using the setnum in the index field and using 0, 1,
  //2, and 3 in the tag field

    uint32_t r0d0 = (setnum << L1_SET_INDEX_SHIFT) + (0 << L1_ADDRESS_TAG_SHIFT);
    uint32_t r0d1 = (setnum << L1_SET_INDEX_SHIFT) + (1 << L1_ADDRESS_TAG_SHIFT);
    uint32_t r1d0 = (setnum << L1_SET_INDEX_SHIFT) + (2 << L1_ADDRESS_TAG_SHIFT);
    uint32_t r1d1 = (setnum << L1_SET_INDEX_SHIFT) + (3 << L1_ADDRESS_TAG_SHIFT);


    for(j=0;j<WORDS_PER_CACHE_LINE;j++)
      write_data[j] = setnum + j;

    l1_insert_line(r0d1, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (status & 0x1) { //writeback, shouldn't occur here
      printf("Error: Writeback occurred when r0d1, address %x, was inserted\n", r0d1);
      exit(1);
    }

    l1_insert_line(r0d0, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (status & 0x1) { //writeback, shouldn't occur here
      printf("Error: Writeback occurred when r0d0, address %x, was inserted\n", r0d0);
      exit(1);
    }


    l1_insert_line(r1d0, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (status & 0x1) { //writeback, shouldn't occur here
      printf("Error: Writeback occurred when r1d0, address %x, was inserted\n", r1d0);
      exit(1);
    }

    l1_insert_line(r1d1, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (status & 0x1) { //writeback, shouldn't occur here
      printf("Error: Writeback occurred when r1d1, address %x, was inserted\n", r1d1);
      exit(1);
    }

  //write to r0d1 at some random offset within the line (where the offset is divisible by 4)

    l1_cache_access(r0d1 + ((rand()%BYTES_PER_CACHE_LINE) & ~0x3), 0, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) {
      printf("Error: Cache miss on writing to r0d1, address %x\n", r0d1);
      exit(1);
    }

  //clear r bits


    l1_clear_r_bits();


  //write to r1d1

    l1_cache_access(r1d1 + ((rand()%BYTES_PER_CACHE_LINE) & ~0x3), 0, WRITE_ENABLE_MASK, NULL, &status);

    if (!(status & 0x1)) {
      printf("Error: Cache miss on writing to r1d1, address %x\n",r1d1);
      exit(1);
    }


  //read from r1d0

    l1_cache_access(r1d0 + ((rand() % BYTES_PER_CACHE_LINE) & ~0x3), ~0x0, READ_ENABLE_MASK, &read_data, &status);

    if (!(status & 0x1)) {
      printf("Error: Cache miss on reading from to r1d0, address %x\n",r1d0);
      exit(1);
    }

  //insert line new1. Check that r0d0 is evicted, no writeback

#define BYTES_PER_CACHE (SETS_PER_CACHE * LINES_PER_SET * BYTES_PER_CACHE_LINE)

    uint32_t new1 = r0d0 + BYTES_PER_CACHE;

    l1_insert_line(new1, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (status & 0x1) { //error, writeback indicated.
      printf("Error: r0d0, address %x, should not have to be written back\n", r0d0);
      exit(1);
    }


   // Now let's read from r0d0 to make sure we get a cache miss:

    l1_cache_access(r0d0, ~0x0, READ_ENABLE_MASK, &read_data, &status);

    if (status & 0x1) {   // cache hit!
      printf("Error: r0d0, address %x, was not evicted. A subsequent read resuted in a cache hit\n", r1d0);
      exit(1);
    }


  //write to new1 so it won't be evicted.

    l1_cache_access(new1, 0, WRITE_ENABLE_MASK, NULL, &status);

  //insert line new2. Check that r0d1 is evicted, writeback.

    uint32_t new2 = r0d1 + BYTES_PER_CACHE;

    l1_insert_line(new2, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);

    if (evicted_writeback_address != r0d1) {
      printf("Error: r0d1, address %x, should have been evicted rather than address %x\n", r0d1,
	     evicted_writeback_address);
      exit(1);
    }

    if (!(status & 0x1)) { //error, writeback not indicated.
      printf("Error: r0d, address %x, should be written back\n", r0d1);
      exit(1);
    }


  //write to new2 so it won't be evicted

    l1_cache_access(new2, 0, WRITE_ENABLE_MASK, NULL, &status);


  //insert line new3. Check that r1d0 is evicted, no writeback. The way to do this is, 
  //after inserting new3, make sure you have a l1 miss when trying to read r1d0.

    uint32_t new3 = r1d0 + BYTES_PER_CACHE;

    l1_insert_line(new3, write_data, &evicted_writeback_address, 
		   evicted_writeback_data, &status);


    if (status & 0x1) { //error, writeback indicated.
      printf("Error: r1d0, address %x, should not be written back when evicted\n", r1d0);
      exit(1);
    }


    // Now let's read from r1d0 to make sure we get a cache miss:

    l1_cache_access(r1d0, ~0x0, READ_ENABLE_MASK, &read_data, &status);

    if (status & 0x1) {   // cache hit!
      printf("Error: r1d0, address %x, was not evicted. A subsequent read resuted in a cache hit\n", r1d0);
      exit(1);
    }


    //write to new3 so it won't be evicted

    l1_cache_access(new3, 0, WRITE_ENABLE_MASK, NULL, &status);

  }

  printf("Passed\n");

}
