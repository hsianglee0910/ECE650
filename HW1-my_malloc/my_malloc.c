//
//  my_malloc.c
//  my_malloc
//
//  Created by Yifan Li on 1/21/18.
//  Copyright © 2018 Yifan Li. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "my_malloc.h"

void *begin = NULL;
unsigned long totalheap = 0;


block_info *get_newblock(size_t size, block_info *lastblock) {
    block_info *newblock = sbrk(0);
    if (sbrk(size+BLOCK_INFO_SIZE) == (void *) -1) {
        fprintf(stderr, "sbrk failed\n");
        return NULL;
    }
    
    newblock->next = NULL;
    newblock->prev = lastblock;
    newblock->blockSize = size;
    
    if (lastblock != NULL) { // had some blocks before
        lastblock->next = newblock; //append the new block
    }
    newblock->isFree = 0;
    
    totalheap += size+BLOCK_INFO_SIZE; //policy test counter

    return newblock;
}

block_info *ff_find_free_region(size_t size, block_info **lastblock_ptr) {
    block_info * curr_block = begin;
    while ( (curr_block != NULL) && !(curr_block->isFree && curr_block->blockSize >= size) ) {
        *lastblock_ptr = curr_block;
        curr_block = curr_block->next;
    }
    return curr_block;
}

void split_block(block_info * b, size_t size) {
  block_info* remainder = (block_info *)((char *)b + BLOCK_INFO_SIZE + size); // new meta position
  
  remainder->next = b->next;
  remainder->prev = b;
  remainder->blockSize = b->blockSize - BLOCK_INFO_SIZE - size;
  remainder->isFree = 1;
  
  b->blockSize = size;
  b->next = remainder;
  if (remainder->next)
    remainder->next->prev = remainder;
}

void *ff_malloc(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    block_info * newblock;
    block_info * lastblock;
    if (begin == NULL) { // very first malloc call
        newblock = get_newblock(size, NULL);
        if (newblock == NULL) {
            return NULL;
        }
        begin = newblock;
    } else { // not the 1st malloc call
        lastblock = begin;
        newblock = ff_find_free_region(size, &lastblock);
	
	if (newblock) {
	  // try to split here
	  if ((newblock->blockSize - size) >= (BLOCK_INFO_SIZE+4))
	    split_block(newblock, size);
	  newblock->isFree = 0;
	} else {
	  // no proper free space found, get a new block
	  newblock = get_newblock(size, lastblock);
	  if (!newblock)
	    return NULL;
	} 	  
    }
    return (newblock+1);
}

block_info *merge_block(block_info * b) {
  if (b->next && b->next->isFree) {
    b->blockSize += (BLOCK_INFO_SIZE + b->next->blockSize);
    b->next = b->next->next;
    if (b->next != NULL)
      b->next->prev = b;
  }
  return b;
}

void ff_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    // find the address of the meta struct
    block_info *block_info_ptr = (block_info *)ptr - 1;

    if (block_info_ptr->isFree) {
        fprintf(stderr, "double free!\n");
        //exit(EXIT_FAILURE);
        return;
    }
    block_info_ptr->isFree = 1;
    //  freespace += block_info_ptr->blockSize+BLOCK_INFO_SIZE;
    if (block_info_ptr->prev && block_info_ptr->prev->isFree)
      block_info_ptr = merge_block(block_info_ptr->prev);
    if (block_info_ptr->next)
      merge_block(block_info_ptr);
    else {
      //
      if (block_info_ptr->prev)
	block_info_ptr->prev->next = NULL;
      else 
	begin = NULL;
    }
}   

block_info *bf_find_free_region(size_t size, block_info **lastblock_ptr) {
    block_info * curr_block = begin;
    size_t difference = ULONG_MAX;
    block_info * bestfit = NULL;
    while (curr_block != NULL) {
        if (curr_block->isFree && curr_block->blockSize >= size) {
            if (curr_block->blockSize-size < difference) {
                difference = curr_block->blockSize-size;
                bestfit = curr_block;
            }
        }
        *lastblock_ptr = curr_block;
        curr_block = curr_block->next;
    }

    return bestfit;
}

void *bf_malloc(size_t size) {
    if (size <= 0) {
        fprintf(stderr, "The requested size should be a positive integer\n");
        return NULL;
    }
    block_info * newblock, *lastblock;
    if (begin == NULL) { // very first malloc call
        newblock = get_newblock(size, NULL);
        if (newblock == NULL) {
            return NULL;
        }
        begin = newblock;
    } else { // not the 1st malloc call
        lastblock = begin;
        newblock = bf_find_free_region(size, &lastblock);
        if (newblock) {
	  //try to split here
	  if ((newblock->blockSize - size) >= (BLOCK_INFO_SIZE+4))
	    split_block(newblock, size);
	  newblock->isFree = 0;
	} else {
	  // no proper free space found, get a new block
	  newblock = get_newblock(size, lastblock);
	  if (!newblock)
	    return NULL;
	} 	  
    }
    return (newblock+1);
}

void bf_free(void *ptr) {
    ff_free(ptr); //same implementation as before
}   


unsigned long get_data_segment_size() {
  return totalheap;
}

unsigned long get_data_segment_free_space_size() {
  unsigned long freespace = 0;
  block_info * curr = begin;
  while (curr != NULL) {
    if (curr->isFree) {
      freespace += curr->blockSize+BLOCK_INFO_SIZE;
    }
    curr = curr->next;
  }
  return freespace;
}
