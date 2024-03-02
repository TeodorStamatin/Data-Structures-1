#include "vma.h"
#include <stdlib.h>
#include <stdio.h>

list_t *ll_create(unsigned int data_size)
{
    list_t *list = malloc(sizeof(list_t));
    DIE(!list, "malloc() failed");

    list->data_size = data_size;
    list->size = 0;
    list->head=NULL;
    list->last=NULL;

    return list;
}

node_t* ll_get_nth_node(list_t* list, unsigned int n) {
	DIE(!list, "malloc() failed");

   node_t *curr;
   curr = list->head;

   if(n >= list->size)
      n = n % list->size;

   for(int i = 0; i < n; i++)
      curr = curr->next;

   return curr;
}

void ll_add_nth_node(list_t* list, unsigned int n, const void* data) {
    if (n > list->size) {

        n = list->size;
    }

    node_t* new_node = (node_t*) malloc(sizeof(node_t));
    new_node->data = (void*) data;
    
    if (n == 0) {

        new_node->prev = NULL;
        new_node->next = list->head;
        if (list->head == NULL) {

            list->last = new_node;
        } else {
            list->head->prev = new_node;
        }
        list->head = new_node;
    } else if (n == list->size) {

        new_node->prev = list->last;
        new_node->next = NULL;
        if (list->last == NULL) {

            list->head = new_node;
        } else {
            list->last->next = new_node;
        }
        list->last = new_node;
    } else {

        node_t* curr_node = list->head;
        for (unsigned int i = 0; i < n; i++) {
            curr_node = curr_node->next;
        }
        new_node->prev = curr_node->prev;
        new_node->next = curr_node;
        curr_node->prev->next = new_node;
        curr_node->prev = new_node;
    }
    
    list->size++;
}

void ll_remove_nth_node(list_t* list, unsigned int n) {

    if (list->size == 0 || n >= list->size) {
        return;
    }

    node_t* current = list->head;

    for (unsigned int i = 0; i < n; i++) {
        current = current->next;
    }

    if (current == list->head) {
        list->head = current->next;
    } else {
        current->prev->next = current->next;
    }

    if (current == list->last) {
        list->last = current->prev;
    } else {
        current->next->prev = current->prev;
    }

    free(current->data);
    free(current);

    list->size--;
}

unsigned int dll_get_size(list_t* list) {
	return list->size;
}

void ll_free(list_t** pp_list) {
	node_t *curr, *tmp;
	DIE(!(*pp_list), "malloc() failed");
    
   curr = (*pp_list)->head;
   while(curr) {
      tmp = curr;
      curr = curr->next;
      free(tmp);
   }

   free(*pp_list);
   *pp_list = NULL;
}

arena_t *alloc_arena(const uint64_t size) {

   arena_t* arena = malloc(sizeof(arena_t));
   DIE(!arena, "malloc() failed");

   arena->arena_size = size;
   arena->alloc_list = ll_create(sizeof(block_t*));

   return arena;
}

void dealloc_arena(arena_t *arena) {

    DIE(!arena, "There is no arena allocated");

    node_t *curr_block = arena->alloc_list->head;

    // luam fiecare block in parte
    while(curr_block) {
        block_t *block = (block_t*)curr_block->data;
        node_t *curr_miniblock = ((list_t *)block->miniblock_list)->head;

        // luam fiecare miniblock in parte
        while(curr_miniblock) {
            miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;

            // daca miniblock-ul are un buffer alocat, il eliberam
            if(miniblock->rw_buffer)
                free(miniblock->rw_buffer);
            
            // eliberam miniblock-ul
            miniblock->rw_buffer = NULL;
            free(miniblock);
            miniblock = NULL;
            curr_miniblock = curr_miniblock->next;
        }
        
        // eliberam lista de miniblock-uri
        list_t *list = ((list_t *)block->miniblock_list);
        free(block);
        block = NULL;
        ll_free(&list);
        curr_block = curr_block->next;
    }
    ll_free(&arena->alloc_list);
    free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size) {

    block_t *block1;
    block_t *block2;
    block_t *block3;
    block_t *block4;

    if(!arena) {
        printf("There is no arena allocated\n");
        return;
    }
    if(!arena->alloc_list) {
        printf("There are no blocks allocated\n");
        return;
    }

    if(address < 0 || address >= arena->arena_size) {
        printf("The allocated address is outside the size of arena\n");
        return;
    }
    if(address + size > arena->arena_size) {
        printf("The end address is past the size of the arena\n");
        return;
    }

    if (arena->alloc_list->head == NULL) { // adaugarea primului block din arena

        block_t *block = malloc(sizeof(block_t));
        block->miniblock_list = ll_create(sizeof(miniblock_t));
        block->size = size;
        block->start_address = address;

        miniblock_t *miniblock = malloc(sizeof(miniblock_t));
        miniblock->size = size;
        miniblock->rw_buffer = malloc(miniblock->size);
        miniblock->start_address = address;
        miniblock->perm = 6;
        
        ll_add_nth_node(arena->alloc_list, 0, block);
        ll_add_nth_node(block->miniblock_list, 0, miniblock);
        return;
    }
    
    // verificam daca zona este deja alocata
    node_t *check_overlap = arena->alloc_list->head;
    while(check_overlap) {

        block_t* block = (block_t*)check_overlap->data;
        if (!(address + size <= block->start_address || block->start_address + block->size <= address)) {
            printf("This zone was already allocated.\n");
            return;
        }
        check_overlap = check_overlap->next;
    }

    node_t *curr_block, *prev_block;
    curr_block = arena->alloc_list->head;
    prev_block = NULL;

    int pos_block = 0;

    // cautam pozitia in lista de block-uri unde trebuie inserat noul block
    while(curr_block && address >= ((block_t*)curr_block->data)->start_address + ((block_t*)curr_block->data)->size) {

        prev_block = curr_block;
        curr_block = curr_block->next;
        pos_block++;
    }

    if(curr_block != NULL && prev_block == NULL) { //avem doar un block si vrem sa inseram inaintea lui
        if(address + size == ((block_t*)curr_block->data)->start_address) { // block ul nou se lipeste inaintea primului block

            block_t *block = (block_t*)curr_block->data;
            block->size = block->size + size;
            block->start_address = address;

            miniblock_t *miniblock = malloc(sizeof(miniblock_t));
            miniblock->size = size;
            miniblock->rw_buffer = malloc(miniblock->size);
            miniblock->start_address = address;
            miniblock->perm = 6;
        
            ll_add_nth_node(block->miniblock_list, 0, miniblock);
            return;
        } else if(address + size < ((block_t*)curr_block->data)->start_address) { // facem un block nou inaintea primului block

                    block_t *block = malloc(sizeof(block_t));
                    block->miniblock_list = ll_create(sizeof(miniblock_t));
                    block->size = size;
                    block->start_address = address;

                    miniblock_t *miniblock = malloc(sizeof(miniblock_t));
                    miniblock->size = size;
                    miniblock->rw_buffer = malloc(miniblock->size);
                    miniblock->start_address = address;
                    miniblock->perm = 6;
                    
                    ll_add_nth_node(arena->alloc_list, 0, block);
                    ll_add_nth_node(block->miniblock_list, 0, miniblock);
                    return;
                }
    }

    if(!curr_block && prev_block) { // am aj la ultimul block
        if(address == ((block_t*)prev_block->data)->start_address + ((block_t*)prev_block->data)->size) { // se adauga la ultimul block

            block_t *block = (block_t*)prev_block->data;
            block->size = block->size + size;

            miniblock_t *miniblock = malloc(sizeof(miniblock_t));
            miniblock->size = size;
            miniblock->rw_buffer = malloc(miniblock->size);
            miniblock->start_address = address;
            miniblock->perm = 6;
        
            ll_add_nth_node(block->miniblock_list, ((list_t*)block->miniblock_list)->size, miniblock);
        return;
        } else { // se adauga ultimul block

            block_t *block = malloc(sizeof(block_t));
            block->miniblock_list = ll_create(sizeof(miniblock_t));
            block->size = size;
            block->start_address = address;

            miniblock_t *miniblock = malloc(sizeof(miniblock_t));
            miniblock->size = size;
            miniblock->rw_buffer = malloc(miniblock->size);
            miniblock->start_address = address;
            miniblock->perm = 6;
            
            ll_add_nth_node(arena->alloc_list, pos_block, block);
            ll_add_nth_node(block->miniblock_list, 0, miniblock);
            return;
        }
    } else { // nu am aj la ultimul block
        
        int option = 0;

        if(address > ((block_t*)prev_block->data)->start_address + ((block_t*)prev_block->data)->size) {
            if(address + size < ((block_t*)curr_block->data)->start_address)
                option = 1; // {...[block]...[block]...[block]...}
            if(address + size == ((block_t*)curr_block->data)->start_address)
                option = 2; // {...[block]...[block][block]...}
        } else {
            if(address == ((block_t*)prev_block->data)->start_address + ((block_t*)prev_block->data)->size) {
                if(address + size < ((block_t*)curr_block->data)->start_address)
                    option = 3; // {...[block][block]...[block]...}
                if(address + size == ((block_t*)curr_block->data)->start_address)
                    option = 4; // {...[block][block][block]...}
            }
        }
        
        switch(option) {
            // {...[block]...[block]...[block]...}
            case 1:

                block1 = malloc(sizeof(block_t));
                block1->miniblock_list = ll_create(sizeof(miniblock_t));
                block1->size = size;
                block1->start_address = address;

                miniblock_t *miniblock1 = malloc(sizeof(miniblock_t));
                miniblock1->size = size;
                miniblock1->rw_buffer = malloc(miniblock1->size);
                miniblock1->start_address = address;
                miniblock1->perm = 6;
                
                ll_add_nth_node(arena->alloc_list, pos_block, block1);
                ll_add_nth_node(block1->miniblock_list, 0, miniblock1);
                break;
            // {...[block]...[block][block]...}
            case 2:

                block2 = (block_t*)curr_block->data;
                block2->size = block2->size + size;
                block2->start_address = address;

                miniblock_t *miniblock2 = malloc(sizeof(miniblock_t));
                miniblock2->size = size;
                miniblock2->rw_buffer = malloc(miniblock2->size);
                miniblock2->start_address = address;
                miniblock2->perm = 6;
            
                ll_add_nth_node(block2->miniblock_list, 0, miniblock2);
                break;
            // {...[block][block]...[block]...}
            case 3:

                block3 = (block_t*)prev_block->data;
                block3->size = block3->size + size;

                miniblock_t *miniblock3 = malloc(sizeof(miniblock_t));
                miniblock3->size = size;
                miniblock3->rw_buffer = malloc(miniblock3->size);
                miniblock3->start_address = address;
                miniblock3->perm = 6;
            
                ll_add_nth_node(block3->miniblock_list, ((list_t*)block3->miniblock_list)->size, miniblock3);
                break;
            // {...[block][block][block]...}
            case 4:

                block4 = (block_t*)prev_block->data;
                block4->size = block4->size + size;

                miniblock_t *miniblock4 = malloc(sizeof(miniblock_t));
                miniblock4->size = size;
                miniblock4->rw_buffer = malloc(miniblock4->size);
                miniblock4->start_address = address;
                miniblock4->perm = 6;
            
                ll_add_nth_node(block4->miniblock_list, ((list_t*)block4->miniblock_list)->size, miniblock4);
                
                // se adauga miniblock-urile din block-ul curent in block-ul anterior
                block_t *block = (block_t*)curr_block->data;
                node_t *curr_miniblock = ((list_t *)block->miniblock_list)->head;

                while(curr_miniblock) {
                    miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;

                    miniblock_t *miniblock_copy = malloc(sizeof(miniblock_t));
                    miniblock_copy->rw_buffer = malloc(128 * sizeof(uint8_t));
                    miniblock_copy->size = miniblock->size;
                    memcpy(miniblock_copy->rw_buffer, miniblock->rw_buffer, miniblock->size);
                    miniblock_copy->start_address = miniblock->start_address;
                    miniblock_copy->perm = miniblock->perm;

                    block4->size = block4->size + miniblock->size;
                
                    ll_add_nth_node(block4->miniblock_list, ((list_t*)block4->miniblock_list)->size, miniblock_copy);
                    
                    if(miniblock->rw_buffer)
                        free(miniblock->rw_buffer);
                    miniblock->rw_buffer = NULL;
                    free(miniblock);
                    miniblock = NULL;
                    curr_miniblock = curr_miniblock->next;
                }
                
                // se sterge block-ul curent
                list_t *list = ((list_t *)block->miniblock_list);
                ll_free(&list);
                ll_remove_nth_node(arena->alloc_list, pos_block);
                break;
            default:
                printf("NOT OK\n");
        }
    }
}

void free_block(arena_t *arena, const uint64_t address) {

    node_t *tmp_mini;
    block_t *new_block;

    if(!arena) {
        printf("There is no arena allocated\n");
        return;
    }
    if(!arena->alloc_list) {
        printf("There are no blocks allocated\n");
        return;
    }

    if(address < 0 || address >= arena->arena_size) {
        printf("Invalid address for free.\n");
        return;
    }

    node_t *curr_block = arena->alloc_list->head;
    int pos_block = 0;
    
    int is_ok = 0;

    // se cauta block-ul in care se afla adresa
    while (curr_block) {
        if (address >= ((block_t*)curr_block->data)->start_address && address < ((block_t*)curr_block->data)->start_address + ((block_t*)curr_block->data)->size) {
            is_ok = 1;
            break;
        }
        curr_block = curr_block->next;
        pos_block++;
    }

    if(is_ok == 0 ) {
        printf("Invalid address for free.\n");
        return;
    }

    block_t *block = (block_t*)curr_block->data;
    node_t *curr_miniblock = ((list_t *)block->miniblock_list)->head;
    is_ok = 0;
    int pos_mini = 0;

    // se cauta miniblock-ul in care se afla adresa
    while(curr_miniblock) {

        int option = 0;

        miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;

        if(miniblock->start_address == address) { // am gasit miniblock ul cerut

            is_ok = 1;

            if(miniblock->start_address == block->start_address) { // primul miniblock din block
                if(miniblock->size == block->size) // block = miniblock
                    option = 1;
                else
                    option = 2;
            } else {
                if(miniblock->start_address + miniblock->size == block->start_address + block->size) // ultimul miniblock din block
                    option = 3;
                else // miniblock din interior
                    option = 4;
            }

            switch(option) {
                case 1:
                    if(miniblock->rw_buffer)
                        free(miniblock->rw_buffer);
                    miniblock->rw_buffer = NULL;
                    free(miniblock);
                    miniblock = NULL;
                    list_t *list1 = ((list_t *)block->miniblock_list);
                    ll_free(&list1);
                    
                    ll_remove_nth_node(arena->alloc_list, pos_block);
                    break;
                case 2:
                    if(miniblock->rw_buffer)
                        free(miniblock->rw_buffer);
                    miniblock->rw_buffer = NULL;

                    block->start_address = miniblock->start_address + miniblock->size;
                    block->size = block->size - miniblock->size;

                    ll_remove_nth_node(block->miniblock_list, 0);
                    break;
                case 3:
                    if(miniblock->rw_buffer)
                        free(miniblock->rw_buffer);
                    miniblock->rw_buffer = NULL;

                    block->size = block->size - miniblock->size;
                    
                    ll_remove_nth_node(block->miniblock_list, pos_mini);
                    break;
                case 4:

                    tmp_mini = curr_miniblock->next;

                    new_block = malloc(sizeof(block_t));


                    new_block->miniblock_list = ll_create(sizeof(miniblock_t));
                    new_block->size = 0;
                    new_block->start_address = miniblock->start_address + miniblock->size;

                    int pos = 0;

                    // se copiaza miniblock-urile din block-ul curent in block-ul nou
                    while(tmp_mini) {

                        miniblock_t *node = (miniblock_t*)tmp_mini->data;
                        miniblock_t *new_miniblock = malloc(sizeof(miniblock_t));
                        new_miniblock->size = node->size;
                        new_miniblock->rw_buffer = malloc(new_miniblock->size);

                        memcpy(new_miniblock->rw_buffer, node->rw_buffer, new_miniblock->size);
                        new_miniblock->start_address = node->start_address;
                        new_miniblock->perm = 6;
                        new_block->size = new_block->size + node->size;

                        ll_add_nth_node(new_block->miniblock_list, pos++, new_miniblock);

                        tmp_mini = tmp_mini->next;
                        
                        if(node->rw_buffer)
                            free(node->rw_buffer);
                        node->rw_buffer = NULL;
                    
                        ll_remove_nth_node(block->miniblock_list, pos_mini + 1);
                    }
                    // se modifica dimensiunea block-ului curent
                    block->size = block->size - new_block->size;
                    block->size = block->size - ((miniblock_t*)curr_miniblock->data)->size;

                    if(miniblock->rw_buffer)
                            free(miniblock->rw_buffer);
                        miniblock->rw_buffer = NULL;
                    
                    // se sterge miniblock-ul curent
                    ll_remove_nth_node(block->miniblock_list, pos_mini);
                    
                    // se adauga block-ul nou in lista de block-uri
                    ll_add_nth_node(arena->alloc_list, pos_block + 1, new_block);
                    break;
                default:
                    printf("NOT OK\n");
            }
            break;
        }
        if(is_ok)
            break;
        pos_mini++;
        curr_miniblock = curr_miniblock->next;
    }

    if(is_ok == 0 ) {
        printf("Invalid address for free.\n");
        return;
    }
}

void read(arena_t *arena, uint64_t address, uint64_t size) {

    if(!arena) {
        printf("There is no arena allocated\n");
        return;
    }
    if(!arena->alloc_list) {
        printf("There are no blocks allocated\n");
        return;
    }

    if(address < 0 || address >= arena->arena_size) {
        printf("Invalid address for read.\n");
        return;
    }
    if(address + size > arena->arena_size) {
        printf("Invalid address for read.\n");
        return;
    }
    
    node_t *curr_block = arena->alloc_list->head;
    
    int is_ok = 0;

    // se cauta block-ul in care se afla adresa
    while (curr_block) {
        if (address >= ((block_t*)curr_block->data)->start_address && address <= ((block_t*)curr_block->data)->start_address + ((block_t*)curr_block->data)->size) {
            is_ok = 1;
            break;
        }
        curr_block = curr_block->next;
    }

    if(is_ok == 0 ) {
        printf("Invalid address for read.\n");
        return;
    }

    // se verifica daca permisiunile sunt corecte
    block_t *block = (block_t*)curr_block->data;
    node_t *curr_miniblock = ((list_t *)block->miniblock_list)->head;

    is_ok = 1;

    while(curr_miniblock) {
        miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;
        if(miniblock->perm != 7 && miniblock->perm != 6 && miniblock->perm != 4) {
            is_ok = 0;
            break;
        }
        curr_miniblock = curr_miniblock->next;
    }

    if(is_ok == 0 ) {
        printf("Invalid permissions for read.\n");
        return;
    }

    curr_miniblock = ((list_t *)block->miniblock_list)->head;

    while(curr_miniblock) { // ajungem la miniblock ul care trebuie
        if(address >= ((miniblock_t*)curr_miniblock->data)->start_address && address < ((miniblock_t*)curr_miniblock->data)->start_address + ((miniblock_t*)curr_miniblock->data)->size)
            break;
        curr_miniblock = curr_miniblock->next;
    }

    int i = 0, j = address, max_read = size;

    if(size > block->size - (address - block->start_address)) {
        printf("Warning: size was bigger than the block size. Reading %ld characters.\n", block->size - (address - block->start_address));
        max_read = block->size - (address - block->start_address);
    }

    // se citeste din miniblock-uri
    while(i < max_read) {

        miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;

        printf("%c", ((char*)miniblock->rw_buffer)[address - miniblock->start_address + i]);

        i++;

        j++;

        if(miniblock->start_address + miniblock->size == j)
            curr_miniblock = curr_miniblock->next;
    }
    printf("\n");
}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data) {
    
    if(!arena) {
        printf("There is no arena allocated\n");
        return;
    }
    if(!arena->alloc_list) {
        printf("There are no blocks allocated\n");
        return;
    }

    if(address < 0 || address >= arena->arena_size) {
        printf("Invalid address for write.\n");
        return;
    }
    if(address + size > arena->arena_size) {
        printf("Invalid address for write.\n");
        return;
    }
    
    node_t *curr_block = arena->alloc_list->head;
    
    int is_ok = 0;

    // se cauta block-ul in care se afla adresa
    while (curr_block) {
        if (address >= ((block_t*)curr_block->data)->start_address && address <= ((block_t*)curr_block->data)->start_address + ((block_t*)curr_block->data)->size) {
            is_ok = 1;
            break;
        }
        curr_block = curr_block->next;
    }

    if(is_ok == 0 ) {
        printf("Invalid address for write.\n");
        return;
    }

    block_t *block = (block_t*)curr_block->data;
    node_t *curr_miniblock = ((list_t *)block->miniblock_list)->head;

    is_ok = 1;
    
    // se verifica daca permisiunile sunt corecte
    while(curr_miniblock) {
        miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;
        if(miniblock->perm != 7 && miniblock->perm != 6 && miniblock->perm != 2) {
            is_ok = 0;
            break;
        }
        curr_miniblock = curr_miniblock->next;
    }

    if(is_ok == 0 ) {
        printf("Invalid permissions for write.\n");
        return;
    }

    curr_miniblock = ((list_t *)block->miniblock_list)->head;

    while(curr_miniblock) { // ajungem la miniblock ul care trebuie
        if(address >= ((miniblock_t*)curr_miniblock->data)->start_address &&  address < ((miniblock_t*)curr_miniblock->data)->start_address + ((miniblock_t*)curr_miniblock->data)->size)
            break;
        curr_miniblock = curr_miniblock->next;
    }

    int i = 0, j = address, max_write = size;

    if(size > block->size - (address - block->start_address)) {
        printf("Warning: size was bigger than the block size. Writing %ld characters.\n", block->size - (address - block->start_address));
        max_write = block->size - (address - block->start_address);
    }

    // se scrie in miniblock-uri
    while(i < max_write) {
        
        miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;

        ((char*)miniblock->rw_buffer)[address - miniblock->start_address + i] = data[i + 1];
        i++;

        j++;
        if(j == miniblock->start_address + miniblock->size)
            curr_miniblock = curr_miniblock->next;
    }
}

void pmap(const arena_t *arena) {

    node_t *curr = arena->alloc_list->head;

    uint64_t allocated_mem = 0;

    // se calculeaza memoria alocata
    int nr = 0;
    while(curr) {
        int n = ((list_t*)((block_t*)curr->data)->miniblock_list)->size;
        allocated_mem = allocated_mem + ((block_t*)curr->data)->size;
        nr +=n;
        curr = curr->next;
    }
   
    printf("Total memory: 0x%lX bytes\n", arena->arena_size);
    printf("Free memory: 0x%lX bytes\n", arena->arena_size - allocated_mem);
    printf("Number of allocated blocks: %d\n", arena->alloc_list->size);
    printf("Number of allocated miniblocks: %d\n", nr);

    node_t *node = arena->alloc_list->head;
    int i = 0;

    // se afiseaza informatiile despre fiecare block
    while(node) {

        i++;
        printf("\n");
        printf("Block %d begin\n", i);

        block_t *block = (block_t*)node->data;

        uint64_t addr1 = block->start_address;
        size_t size = block->size;
        uint64_t addr2 = addr1 + size;
        printf("Zone: 0x%lX - 0x%lX\n", addr1,addr2);

        node_t *mini = ((list_t*)block->miniblock_list)->head;

        int j = 0;

        // se afiseaza informatiile despre fiecare miniblock
        while(mini) {

            j++;

            miniblock_t *miniblock = (miniblock_t*)mini->data;

            uint64_t addr3 = miniblock->start_address;
            size_t size = miniblock->size;
            uint64_t addr4 = addr3 + size;
            printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| ", j, addr3, addr4);

            // se afiseaza permisiunile
            switch(miniblock->perm) {
                case 6:
                    printf("RW-\n");
                    break;
                case 0:
                    printf("---\n");
                    break;
                case 2:
                    printf("-W-\n");
                    break;
                case 4:
                    printf("R--\n");
                    break;
                case 7:
                    printf("RWX\n");
                    break;
                case 1:
                    printf("--X\n");
                    break;
                case 3:
                    printf("-WX\n");
                    break;
                case 5:
                    printf("R-X\n");
                    break;
                default:
                    break;
            }
            mini = mini->next;
        }

        node = node->next;

        printf("Block %d end\n", i);
    }
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission) {
    
    int option = 0;
    
    // se verifica daca permisiunile sunt corecte
    if(strcmp(permission, " PROT_EXECUTE") == 0)
        option = 1;
    else if(strcmp(permission, " PROT_NONE") == 0)
        option = 0;
    else if(strcmp(permission, " PROT_READ | PROT_WRITE | PROT_EXEC") == 0)
        option = 7;
    else if(strcmp(permission, " PROT_WRITE") == 0)
        option = 2;
    else if(strcmp(permission, " PROT_WRITE | PROT_READ") == 0)
        option = 6;
    else if(strcmp(permission, " PROT_READ") == 0)
        option = 4;
    else if(strcmp(permission, " PROT_READ | PROT_EXEC") == 0)
        option = 5;
    else if(strcmp(permission, " PROT_WRITE | PROT_EXEC") == 0)
        option = 3;
    
    node_t *curr = arena->alloc_list->head;

    int is_ok = 0;

    // se verifica daca adresa este valida
    while(curr) {
        if(address >= ((block_t*)curr->data)->start_address && address <= ((block_t*)curr->data)->start_address + ((block_t*)curr->data)->size) {
            is_ok = 1;
            break;
        }
        curr = curr->next;
    }

    if(is_ok == 0) {
        printf("Invalid address for mprotect.\n");
        return;
    }

    // se verifica daca miniblock-ul exista
    block_t *block = (block_t*)curr->data;
    node_t *curr_miniblock = ((list_t*)block->miniblock_list)->head;
    is_ok = 0;

    while(curr_miniblock) {
        if(address == ((miniblock_t*)curr_miniblock->data)->start_address) {
            is_ok = 1;
            break;
        }
        curr_miniblock = curr_miniblock->next;
    }

    if(is_ok == 0) {
        printf("Invalid address for mprotect.\n");
        return;
    }
    // se schimba permisiunile din miniblock
    miniblock_t *miniblock = (miniblock_t*)curr_miniblock->data;
    miniblock->perm = option;
}
