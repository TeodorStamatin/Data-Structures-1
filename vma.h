#pragma once
#include <inttypes.h>
#include <stddef.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);				        \
		}							\
	} while (0)

typedef struct node_t
{
	void* data;
	struct node_t *prev, *next;
} node_t;

typedef struct list_t
{
   node_t* head;
   node_t* last;
   unsigned int data_size;
   unsigned int size;
} list_t;

typedef struct {
   uint64_t start_address; 
   size_t size; 
   void* miniblock_list;
} block_t;

typedef struct {
   uint64_t start_address;
   size_t size;
   uint8_t perm;
   void* rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

list_t* ll_create(unsigned int data_size);
node_t* ll_get_nth_node(list_t* list, unsigned int n);
void ll_add_nth_node(list_t* list, unsigned int n, const void* data);
void ll_remove_nth_node(list_t* list, unsigned int n);
unsigned int ll_get_size(list_t* list);
void ll_free(list_t** pp_list);

arena_t* alloc_arena(const uint64_t size);
void dealloc_arena(arena_t* arena);

void alloc_block(arena_t* arena, const uint64_t address, const uint64_t size);
void free_block(arena_t* arena, const uint64_t address);

void read(arena_t* arena, uint64_t address, uint64_t size);
void write(arena_t* arena, const uint64_t address,  const uint64_t size, int8_t *data);
void pmap(const arena_t* arena);
void mprotect(arena_t* arena, uint64_t address, int8_t *permission);
