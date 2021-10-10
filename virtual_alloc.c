#include "virtual_alloc.h"
#include "virtual_sbrk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define END_INDEX 255
#define ALLOC 70

/*
This function takes in the heapstart, and initial virtual heap
size, and minimum virtual heap size, and initialises the data structure,
and prepares the virtual heap to be used.
The data structure that is being used is an array of type uint8_t.
Each array index represents one block of heap. Free elements are 
represented based on the size of the block. If size is 2^j, then the
array stores j. Allocated blocks store 70 + j.
Reason for this implementation - maximum value of j is 64, and uint8_t 
can hold 256 integers, hence we can easily represent both allocated
and free blocks. 
Value 255 in array denotes the end of array, and first index of array is
 reserved for minimum size of heap.
 
 parameters:
 heapstart - the address where the heap starts (void*)
 initial_size - the initial size of virtual heap (uint8_t)
 min_size - the minimum size of virtual heap (uint8_t)
 
 return:
 void return type
*/
void init_allocator (void * heapstart, uint8_t initial_size, uint8_t min_size) {

    if (initial_size < min_size) {
       perror ("Initial size cannot be less than the minimum block size\n");
    	exit (0);
    }
    
    uint64_t heap_length = 1 << initial_size;
    void* success = virtual_sbrk ((1 << initial_size) + 4);
    if (success == (void *)(-1)) {
        perror("virtual sbrk failed!\n");
    	exit(0);
    }
    uint8_t *initial = (uint8_t *) heapstart;
    *initial = initial_size;
    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);
   
    buddy [0] = min_size;
    buddy [1] = initial_size;
    buddy [2] = END_INDEX;
}

/*
This function takes in the heapstart, and index of the block to be split
in our data structure as arguments, and splits the block into two buddies.

parameters:
heapstart - the address where the heap starts (void*)
index - index of the block to be split in our data structure (uint64_t)

return: (int)
on failure - it returns -1
on success - it returns the index of the first buddy created from
the split
*/
int buddy_merge (void* heapstart, uint64_t index) {

	uint8_t *initial = (uint8_t *) heapstart;
	uint8_t initial_size = *initial;
	uint64_t heap_length = 1 << initial_size;
	uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);
   
	uint32_t current = buddy [index];
	uint32_t next = buddy [index + 1];

	if (buddy [index] == END_INDEX) {
	  	return -1;
	}
	 
	 // merging with the following/ next block 
	if (next == current && next <= ALLOC) {
	  	buddy [index] = next + 1;
	  	uint32_t t = index + 1;
	  	
	  	while (buddy [t] != END_INDEX) {
	  		buddy [t] = buddy [t + 1];
	  		t += 1;		
	  	}
	  	buddy [t - 1] = END_INDEX;
		void* success = virtual_sbrk (-1);
    		if (success == (void *)(-1)) {
        		perror("virtual sbrk failed!\n");
    			exit(0);
    		}
	  	return index;
	}
	  
	// cannot merge with previous block if index is 1, because index
	// 0 is reserved for minimum block size.  
	if (index == 1) {
	  	return -1;
	}
	
	// merging with the previous block
	uint32_t previous = buddy [index - 1];
	if (current == previous && current <= ALLOC) {
	  	buddy [index - 1] = previous + 1;
	  	uint32_t t = index;
	
	  	while (buddy [t] != END_INDEX) {
	  		buddy [t] = buddy [t + 1];
	  		t += 1;	
	  	}
	  	buddy [t - 1] = END_INDEX;
		void* success = virtual_sbrk (-1);
    		if (success == (void *)(-1)) {
        		perror("virtual sbrk failed!\n");
    			exit(0);
    		}
	  	return index - 1;
	  }
	
	return -1;
}

/*
This function takes in the heapstart, and index of the first buddy
to be merged, and merges the buddies to form an unallocated block 
if they both are free.

parameters:
heapstart - the address where the heap starts (void*)
index - index of the first buddy to be merged in our data structure
 (uint64_t)

return: 
it merges the buddies if possible. Return type is void.
*/
void buddy_split (void* heapstart, uint64_t index) {

	uint8_t *initial = (uint8_t *) heapstart;
	uint8_t initial_size = *initial;
	uint64_t heap_length = 1 << initial_size;
	uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);

	if (buddy [index] >= ALLOC) { //only free can be split
		return;
	}
	
	uint32_t i = 1;
	while (buddy [i] != END_INDEX) {
	    	i += 1; 	
	}
	void* success = virtual_sbrk (1);
	if (success == (void *)(-1)) {
		perror("virtual sbrk failed!\n");
	    	exit(0);
	}
	// now i is last index
	buddy [i + 1] = END_INDEX;
	uint32_t x = 0;
	
	// moving contents of block one index ahead to create space
	// for new block
	for (x = i; x > index; x --) {
	    buddy [x] = buddy [x - 1];
	}
	
	// splitting the block
	buddy [index] = buddy [index + 1] - 1;	
	buddy [index + 1] -= 1;
}

/*
This function takes in the heapstart, and size of the block, and finds
if there is any unallocated block of given size avaialable.

parameters:
heapstart - the address where the heap starts (void*)
index - size of the block to be found (uint64_t)

return: (int)
on failure - it returns 0
on success - it returns the index of the unallocated block of given size.
*/
int leftmost_x_block_index (void * heapstart, uint32_t j) {

	    uint8_t *initial = (uint8_t *) heapstart;
	    uint8_t initial_size = *initial;
	    uint64_t heap_length = 1 << initial_size;
	    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);

    	    uint32_t i = 1;
    	    
	    while (buddy [i] != END_INDEX) {
	    
	        if (buddy [i] <= ALLOC) {
	    	uint64_t size = 1 << buddy[i];
	    		
	    		// finding block of size j, and returning its index
	    		if (size == j) {
	    			return i;
	    		} 	
	    	}
	    	
	    i += 1;   
    	}
	    return 0;
}


/*
This function takes in the heapstart, and size of the block, and finds
if there is any unallocated block of given size avaialable. If available,
then it returns the address of this block in virtual_heap.
This function is used to check if an unallocated block of size 2^j is 
available or not. And if available, its address is returned by malloc.

parameters:
heapstart - the address where the heap starts (void*)
j - size of the block to be found (uint32_t)

return: (void*)
on failure - it returns (void *)(-1).
on success - it returns the address of the unallocated block of given
size in virtual heap.
*/
void * leftmost_j_block (void * heapstart, uint32_t j) {
		
	    uint8_t *initial = (uint8_t *) heapstart;
	    uint8_t initial_size = *initial;
	    uint64_t heap_length = 1 << initial_size;
	    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);
    	    uint32_t i = 1;

	    while (buddy [i] != END_INDEX) {
	        if (buddy [i] <= ALLOC) {

	    		uint64_t size = 1 << buddy[i];
	    		if (size == j) {
	    			uint64_t offset = 0;
	    			uint32_t g = 0;
	    			
	    			// Calculating appropriate address of new 
	    			// allocated block.
	    			for (g = 1; g < i; g ++) {
	    		
	    				uint32_t value = buddy [g];
	    				if (buddy [g] >= ALLOC) {
	    					value -= ALLOC;
	    				}
	    				offset += (1 << value);
	    			}

	    			buddy [i] += ALLOC;
	    			return (void*) (heapstart + 1 + offset);
	    		}
	    
	    	}
	    i += 1; 
    	}
	return (void *)(-1);
}


/*
This function takes in the heapstart, and size of the block, and
allocates a block if possible

parameters:
heapstart - the address where the heap starts (void*)
size - size of the block to be allocated (uint32_t)

return: (void*)
on failure - it returns NULL.
on success - it returns the address of block of given size in virtual heap.
*/
void * virtual_malloc (void * heapstart, uint32_t size) {

    uint8_t *initial = (uint8_t *) heapstart;
    uint8_t initial_size = *initial;
    uint64_t heap_length = 1 << initial_size;
    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);

    uint64_t i = 0;
    uint64_t lower = 0; // j - 1
    uint64_t upper = 1; // j
    uint64_t min = buddy [0];

    if (size == 0 ) {
    	return NULL;
    }
    
    if (size > heap_length) {
     	return NULL;
    }
    
    if (size <= (1 << min) ) {
    	upper = min;
    	
    } else {
    	while (1 > 0) {
    	lower = i;
    	upper = i + 1;
    		if ( (size > (1 << lower)) && (size <= (1 << upper))) {
    			break;
    		}
    	i += 1;
    	}
    }

    // checking if unallocated block of size 2^j bytes exists or not.
    uint64_t two_j = 1 << upper;
    void* result = leftmost_j_block (heapstart, two_j);
    if (result != (void *) (-1)) {
    	return result;
    }
    
    uint32_t y = 1;
    for (y=1; y<=initial_size; y++) {
        // finding block of size 2^(j + y), and splitting it till required.
    	uint32_t x = 0;
    	for (x = y; x > 0; x --){
    	
    		two_j = 1 << (x + upper);
    		uint64_t index = leftmost_x_block_index (heapstart, two_j);
    		// if block found, then we need to split it.
    		if (index == 0) {
    			break;
    		} else {
    			buddy_split (heapstart, index);
    		}
    	}
    	
    	// checking if we were able to find a block of size 2^j or not.
    	// else we repeat the steps for block of size 2^(j + y + 1)
    	two_j = 1 << upper;
    	result = leftmost_j_block (heapstart, two_j);
    	if (result != (void *) (-1)) {
    		return result;
    	}
 
    }
    return NULL;
}

/*
This function takes in the heapstart, and ptr of the block, and
deallocates the block if possible

parameters:
heapstart - the address where the heap starts (void*)
ptr - address of block to be deallocated (ptr*)

return: (int)
on failure - it returns 1.
on success - it returns 0.
*/
int virtual_free(void * heapstart, void * ptr) {

    uint8_t *initial = (uint8_t *) heapstart;
    uint8_t initial_size = *initial;
    uint64_t heap_length = 1 << initial_size;
    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);
	
    uint64_t diff = ptr - heapstart - 1;
    uint64_t index = 0;
    uint64_t sum = 0;
    
    uint32_t i = 0;
    
    // finding the index of the required block in the data structure
    for (i = 1; i <= initial_size; i++) {
    
    	uint32_t temp = buddy [i];
    	if (temp >= ALLOC) {
    		temp -= ALLOC;
    	}
    	
    	sum += (1 << temp);
    	if (sum == diff) {
    		index = i + 1;
    		break;
    	}
    }
    
    if (diff == 0) {
    	index = 1;
    }
    
    // freeing the block
    if (buddy [index] <= ALLOC) {
    	return 1;
    } else {
    	buddy [index] -= ALLOC;
    }
    
    // merging the buddies, till possible.
    while (1 > 0) {

    	int64_t success = buddy_merge (heapstart, index);
    	if (success == -1) {
    		break;
    	} else {
    		index = success;
    	}
    	
    }
    return 0;
}
/*
This function copies given number of bytes from one array to another.

parameters:
buddy - the destination array where contents need to copied (uint8_t*)
temp - the source array from where contents need to copied (uint8_t*)
temp_c - the number of bytes to be copied (uint32_t)

return: void return type
*/
void copy (uint8_t* buddy, uint8_t* temp, uint32_t temp_c) {
	uint32_t i = 0;
	for (i = 0; i < temp_c; i ++) {
		buddy [i] = temp [i];
	}
}

/*
This function takes in the heapstart, ptr of the block, and new size, and
reallocates the block if possible.

parameters:
heapstart - the address where the heap starts (void*)
ptr - address of block to be reallocated (ptr*)
size - number of bytes to be reallocated (uint32_t)

return: (void*)
on failure - it returns NULL.
on success - it returns new address of reallocated block.
*/
void * virtual_realloc(void * heapstart, void * ptr, uint32_t size) {

    uint8_t *initial = (uint8_t *) heapstart;
    uint8_t initial_size = *initial;
    uint64_t heap_length = 1 << initial_size;
    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);

    uint32_t temp_c = 0;
    uint8_t temp [13000];
    
    // copying the contents of data structure temporarily.
    while (buddy [temp_c] != END_INDEX) {
    	temp [temp_c] = buddy [temp_c];
    	temp_c += 1;	
    }
    
    // deallocating the specified block of memory
    uint32_t result = virtual_free (heapstart, ptr);
    if (result != 0) {
    	copy (buddy, temp, temp_c) ;
    	return NULL;
    }
    
    // if ptr is NULL, and size is 0, then we allocate a block of
    // specified size.
    if (ptr == NULL && size != 0) {
    	void *res = virtual_malloc (heapstart, size);
    	if (res != NULL) {
    		memmove (res, ptr, size);
    		return res;
    	}
    }
    
    // if ptr is NULL, and size is zero, then we return NULL, and restore
    // the contents of our data structure
    if (ptr == NULL && size == 0) {
    	copy (buddy, temp, temp_c);
    	return NULL;
    }
    
    if (ptr != NULL && size == 0) {
    	// Act as virtual free only, and return NULL
    	return NULL;
    }
    
    // If all these situations are not true, then we allocate a block
    // of given size.
    void* res = virtual_malloc (heapstart, size);
    if (res != NULL) {
    	memmove (res, ptr, size);
    	return res;
    }
    
    // if the program reaches this point, then that means the block
    // cannot be reallocated. In this case we restore the contents
    // of data structure, and return.
    copy (buddy, temp, temp_c);
    return NULL;
 }
 
/*
This function takes in the heapstart, and prints the current state of 
buddy allocator.

parameters:
heapstart - the address where the heap starts (void*)

return: void return type.
*/
void virtual_info(void * heapstart) {
    uint8_t *initial = (uint8_t *) heapstart;
    uint8_t initial_size = *initial;
    uint64_t heap_length = 1 << initial_size;
    uint8_t *buddy = (uint8_t *) (heapstart + heap_length + 1);

    uint32_t i = 1;
    while (buddy [i] != END_INDEX) {
        
        // finding the actual size of allocated block, by subtracting
        // the offset 70.
    	uint32_t temp = buddy [i];
    	if (buddy [i] >= ALLOC) {
    		temp -= ALLOC;
    	}
    
    	uint64_t size = 1 << temp;
    	
    	if (buddy[i] < ALLOC) {
    		printf ("free %lu\n", size);
    	} else {
    		printf ("allocated %lu\n", size);
    	}
    	
    	i += 1;
    	
    }
    
}


















