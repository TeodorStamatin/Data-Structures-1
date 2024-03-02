#include "vma.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stddef.h> 
#define LENGHT 64

int main(void) {
    char command[LENGHT];
    int ok = 1;
    arena_t *arena;
    while (ok) {
        scanf("%s", command);
        if(strcmp(command, "ALLOC_ARENA") == 0) {

            uint64_t size;
            scanf("%ld", &size);
            arena = alloc_arena(size);
        } else if (strcmp(command, "DEALLOC_ARENA") == 0) {
            
            dealloc_arena(arena);
            ok = 0;
        } else if (strcmp(command, "ALLOC_BLOCK") == 0) {

            uint64_t size, address;
            scanf("%ld%ld", &address, &size);
            alloc_block(arena, address, size);
        } else if (strcmp(command, "FREE_BLOCK") == 0) {

            uint64_t address;
            scanf("%ld", &address);
            free_block(arena, address);
        } else if (strcmp(command, "WRITE") == 0) {
            
            uint64_t size, address;
            scanf("%ld%ld", &address, &size);
            char c, buffer[size + 1];
            for(int i = 0; i < size + 1; i++) {
                scanf("%c", &c);
                buffer[i] = c;
            }
            write(arena, address, size, (int8_t*) buffer);
        } else if (strcmp(command, "READ") == 0) {
            
            uint64_t size, address;
            scanf("%ld%ld", &address, &size);
            read(arena, address, size);
        } else if (strcmp(command, "PMAP") == 0) {
            pmap(arena);
        } else if (strcmp(command, "READ_BLOCK") == 0) {
            
            uint64_t size, address;
            scanf("%ld%ld", &address, &size);
            read(arena, address, size);
        } else if (strcmp(command, "MPROTECT") == 0) {
            
            uint64_t address;
            char perm[LENGHT];
            scanf("%ld", &address);
            fgets(perm, LENGHT, stdin);
            perm[strlen(perm) - 1] = '\0';
            mprotect(arena, address, perm);
        } else printf("Invalid command. Please try again.\n");
    }
    return 0;
}
