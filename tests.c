#include "virtual_alloc.h"
#include "virtual_sbrk.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "cmocka.h"
#include "virtual_alloc.h"

/*Each test case checks for the return values of the functions called,
the program break, contents of the memory, and  virtual info result.
Details about all the tests are mentioned in testinfo.txt*/

void * virtual_heap;
void * heap_start;
void * program_break;
char * expected;
uint64_t size;
uint64_t current_size;

void * virtual_sbrk (int32_t increment) {
    if ((current_size + increment) >= size) {
    	return (void *)(-1);
    }
    void* temp = program_break;
    program_break = temp + increment;
    current_size = current_size + increment;
    return temp;
}

static int initialise (void** state) {
    virtual_heap = malloc (10000000);
    heap_start = virtual_heap;
    program_break = heap_start;
    size = 10000000;
    current_size = 0;
    return 0;
}

static int reset (void** state) {
    free (virtual_heap);
    return 0;
}

static void test_virtual_info () {

    //Redirect output to temp file
    freopen ("out", "w", stdout);
    virtual_info (heap_start);
    freopen ("/dev/tty", "w", stdout);
    FILE *fp = fopen ("out", "r");
    char actual [1000] = "";
    char line [50];
    
    if (fp == NULL) {
    	;
    }
    while (fgets (line, 50, fp) != NULL){ 
    	strcat (actual, line);
    }
    fclose (fp);
    fp = NULL;
    fclose (fopen ("out", "w"));
    assert_string_equal (actual, expected);
}

static void add_value_malloc (void* ptr, uint64_t size) {
    char* p = (char*) ptr;
    uint32_t i = 0;
    for (i = 0; i < size; i ++) {
    	p [i] = 'A';
    }
}

static void check_value_realloc (void* ptr, uint64_t size) {
    char* p = (char*) ptr;
    uint32_t i = 0;
    for (i = 0; i < size; i ++) {
    	if (p [i] != 'A') {
    		assert_true (0);
    	}
    }
}

static void test_program_break (void* ptr) {
	if (ptr == NULL) {
		return;
	}
	if ((ptr - heap_start) < 0) {
		assert_true (0);
	}
	
	if ((program_break - ptr) < 0) {
		assert_true (0);
	}
}

static void test_init_allocator (void** state) {
    init_allocator (heap_start, 15, 12);
    expected = "free 32768\n";
    test_virtual_info ();
}

static void test_malloc_simple (void** state) {
    init_allocator (heap_start, 15, 12);
    test_program_break ( virtual_malloc (virtual_heap, 1000));
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\n";
    test_virtual_info();
}

static void test_malloc_multiple (void** state) {
    init_allocator (heap_start, 15, 12);
    test_program_break ( virtual_malloc (virtual_heap, 1000));
    test_program_break ( virtual_malloc (virtual_heap, 2000));
    test_program_break ( virtual_malloc (virtual_heap, 1000));
    
    expected = "allocated 4096\nallocated 4096\nallocated 4096\nfree 4096\nfree 16384\n";
    test_virtual_info ();
}

static void test_free_simple (void** state) {
    init_allocator (heap_start, 18, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    virtual_free (virtual_heap, result);
    
    expected = "free 262144\n";
    test_virtual_info ();
}

static void test_free_multiple (void** state) {
    init_allocator (heap_start, 15, 12);
    test_program_break( virtual_malloc (virtual_heap, 1000));
    void* result = virtual_malloc (virtual_heap, 2000);
    test_program_break (result);
    void* result2 = virtual_malloc (virtual_heap, 1000);
    test_program_break (result2);
    virtual_free (virtual_heap, result);
    result = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    test_program_break (virtual_malloc (virtual_heap, 500));
    virtual_free (virtual_heap, result);
    virtual_free (virtual_heap, result2);
    
    expected = "allocated 4096\nallocated 4096\nfree 8192\nfree 16384\n";
    test_virtual_info ();
}

static void test_malloc_not_possible (void** state) {
    init_allocator (heap_start, 16, 12);
    virtual_malloc (virtual_heap, 1000);
    void* result = virtual_malloc (virtual_heap, 200000);
    if (result != NULL) {
         assert_true (0);
    }
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\n";
    test_virtual_info ();
}

static void test_free_not_possible (void** state) {
    init_allocator (heap_start, 20, 12);
    test_program_break (virtual_malloc (virtual_heap, 1000));
    void* result = virtual_malloc (virtual_heap, 200000);
    test_program_break (result);
    int success = virtual_free (virtual_heap, result + 1);
    if (success != 1) {
         assert_true (0);
    }
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nallocated 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_realloc_simple (void** state) {
    init_allocator (heap_start, 14, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    
    expected = "allocated 8192\nfree 8192\n";
    test_virtual_info();
}

static void test_realloc_complex (void** state) {
    init_allocator (heap_start, 19, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    add_value_malloc (result, 8192);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 8192);
    void* result2 = virtual_malloc (virtual_heap, 3000);
    test_program_break (result2);
    add_value_malloc (result2, 8192);
    virtual_free (virtual_heap, result2);
    test_program_break (virtual_malloc (virtual_heap, 3000));
    test_program_break (virtual_malloc (virtual_heap, 1600));
    void* result3 = virtual_malloc (virtual_heap, 2200);
    test_program_break (result3);
    add_value_malloc (result3, 4096);
    result3 = virtual_realloc (virtual_heap, result3, 15000);
    test_program_break (result3);
    check_value_realloc (result3, 4096);
    
    expected = "allocated 4096\nallocated 4096\nallocated 8192\nallocated 4096\nfree 4096\nfree 8192\nallocated 16384\nfree 16384\nfree 65536\nfree 131072\nfree 262144\n";
    test_virtual_info ();
}

static void test_realloc_freed_block (void** state) {
    init_allocator (heap_start, 19, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    add_value_malloc (result, 8192);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 8192);
    virtual_free (virtual_heap, result);
    result = virtual_realloc (virtual_heap, result, 15000);
    test_program_break (result);
    if (result != NULL) {
    	assert_true (0);
    }
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\n";
    test_virtual_info ();
}

static void test_realloc_not_possible (void** state) {
    init_allocator (heap_start, 19, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 11115000);
    test_program_break (result);
    if (result != NULL) {
    	assert_true (0);
    }
    
    expected = "allocated 4096\nfree 4096\nallocated 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\n";
    test_virtual_info ();
}

static void test_free_realloced_block (void** state) {
    init_allocator (heap_start, 19, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    void* result2 = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    add_value_malloc (result2, 8192);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result2, 1600);
    test_program_break (result);
    check_value_realloc (result, 1024);
    virtual_free (virtual_heap, result2);
    
    expected = "allocated 8192\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\n";
    test_virtual_info ();
}

static void test_realloc_increment_decrement (void** state) {
    init_allocator (heap_start, 23, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 1600);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 16000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 100);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 8000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    
    expected = "allocated 8192\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\nfree 1048576\nfree 2097152\nfree 4194304\n";
    test_virtual_info ();
}

static void test_malloc_zero (void** state) {
    init_allocator (heap_start, 20, 12);
    void* result = virtual_malloc (virtual_heap, 0);
    test_program_break (result);
    if (result != NULL) {
    	assert_true (0);
    }
    
    expected = "free 1048576\n";
    test_virtual_info ();
}

static void test_malloc_one (void** state) {
    init_allocator (heap_start, 20, 12);
    void* result = virtual_malloc (virtual_heap, 1);
    test_program_break (result);
    if (result != (heap_start + 1)) {
    	assert_true (0);
    }
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_malloc_initial_size (void** state) {
    init_allocator (heap_start, 20, 12);
    void* result = virtual_malloc (virtual_heap, 1048576);
    test_program_break (result);
    
    expected = "allocated 1048576\n";
    test_virtual_info ();
}

static void test_malloc_minimum (void** state) {
    init_allocator (heap_start, 20, 12);
    void* result = virtual_malloc (virtual_heap, 4096);
    test_program_break (result);
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_malloc_minimum_zero (void** state) {
    init_allocator (heap_start, 20, 0); //min block 2^0 = 1
    void* result = virtual_malloc (virtual_heap, 1); //2^0 = 1
    test_program_break (result);
    
    expected = "allocated 1\nfree 1\nfree 2\nfree 4\nfree 8\nfree 16\nfree 32\nfree 64\nfree 128\nfree 256\nfree 512\nfree 1024\nfree 2048\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_malloc_minimum_one (void** state) {
    init_allocator (heap_start, 20, 1); //min block 2^1 = 2
    void* result = virtual_malloc (virtual_heap, 2); 
    test_program_break (result);
    
    expected = "allocated 2\nfree 2\nfree 4\nfree 8\nfree 16\nfree 32\nfree 64\nfree 128\nfree 256\nfree 512\nfree 1024\nfree 2048\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_malloc_minimum_initial_size (void** state) {
    init_allocator (heap_start, 13, 13); 
    void* result = virtual_malloc (virtual_heap, 8192); //2^13 = 8192
    test_program_break (result);
    
    expected = "allocated 8192\n";
    test_virtual_info ();
}

static void test_realloc_one (void** state) {
    init_allocator (heap_start, 10, 7);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 1);
    test_program_break (result);
    check_value_realloc (result, 1024);
    
    expected = "allocated 128\nfree 128\nfree 256\nfree 512\n";
    test_virtual_info ();
}

static void test_realloc_zero (void** state) {
    init_allocator (heap_start, 10, 7);
    void* result = virtual_malloc (virtual_heap, 5);
    test_program_break (result);
    result = virtual_realloc (virtual_heap, result, 0);
    test_program_break (result);
    if (result != NULL) {
    	assert_true (0);
    }
    
    expected = "free 1024\n";
    test_virtual_info ();
}

static void test_realloc_initial_size (void** state) {
    init_allocator (heap_start, 10, 7);
    void* result = virtual_malloc (virtual_heap, 5);
    test_program_break (result);
    add_value_malloc (result, 128);
    result = virtual_realloc (virtual_heap, result, 1024);
    test_program_break (result);
    check_value_realloc (result, 128);
    
    expected = "allocated 1024\n";
    test_virtual_info ();
}

static void test_realloc_minimum (void** state) {
    init_allocator (heap_start, 20, 12);
    void* result = virtual_malloc (virtual_heap, 1001);
    test_program_break (result);
    add_value_malloc (result, 4096);//min = 2^12 = 4096
    result = virtual_realloc (virtual_heap, result, 4096);
    test_program_break (result);
    check_value_realloc (result, 4096);
    
    expected = "allocated 4096\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_realloc_minimum_zero (void** state) {
    init_allocator (heap_start, 20, 0); //min block 2^0 = 1
    void* result = virtual_malloc (virtual_heap, 1000); 
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 1); //2^0 = 1
    test_program_break (result);
    check_value_realloc (result, 1); // truncated size
    
    expected = "allocated 1\nfree 1\nfree 2\nfree 4\nfree 8\nfree 16\nfree 32\nfree 64\nfree 128\nfree 256\nfree 512\nfree 1024\nfree 2048\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_realloc_minimum_one (void** state) {
    init_allocator (heap_start, 20, 1); //min block 2^1 = 2
    void* result = virtual_malloc (virtual_heap, 2000); 
    test_program_break (result);
    add_value_malloc (result, 2048);
    result = virtual_realloc (virtual_heap, result, 2);
    test_program_break (result);
    check_value_realloc (result, 2); //truncated size
    
    expected = "allocated 2\nfree 2\nfree 4\nfree 8\nfree 16\nfree 32\nfree 64\nfree 128\nfree 256\nfree 512\nfree 1024\nfree 2048\nfree 4096\nfree 8192\nfree 16384\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\n";
    test_virtual_info ();
}

static void test_realloc_minimum_initial_size (void** state) {
    init_allocator (heap_start, 13, 13); 
    void* result = virtual_malloc (virtual_heap, 1192); 
    test_program_break (result);
    add_value_malloc (result, 2048);
    void* result2 = virtual_realloc (virtual_heap, result, 8192); //2^13 = 8192
    test_program_break (result2);
    check_value_realloc (result2, 2048);
    
    expected = "allocated 8192\n";
    test_virtual_info ();
}

static void test_integrated (void** state) {
    init_allocator (heap_start, 22, 13); 
    void* result = virtual_malloc (virtual_heap, 1192); 
    test_program_break (result);
    add_value_malloc (result, 2048);
    void* result2 = virtual_realloc (virtual_heap, result, 8192); //2^13 = 8192
    test_program_break (result2);
    check_value_realloc (result2, 2048);
    result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result2 = virtual_malloc (virtual_heap, 5000);
    test_program_break (result);
    add_value_malloc (result2, 8192);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result2, 1600);
    test_program_break (result);
    check_value_realloc (result, 1024);
    virtual_free (virtual_heap, result2);
    result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 1600);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 16000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 100);
    test_program_break (result);
    check_value_realloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 8000);
    test_program_break (result);
    check_value_realloc (result, 1024);
    
    expected = "allocated 8192\nallocated 8192\nallocated 8192\nfree 8192\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\nfree 1048576\nfree 2097152\n";
    test_virtual_info ();
}

static void test_double_free (void** state) {
    init_allocator (heap_start, 16, 12);
    void* result = virtual_malloc (virtual_heap, 1000);
    test_program_break(result);
    virtual_free (virtual_heap, result);
    int success = virtual_free (virtual_heap, result);
    if (success != 1) {
    	assert_true (0);
    }
    
    expected = "free 65536\n";
    test_virtual_info ();
}

static void test_integrated_long (void** state) {
    init_allocator (heap_start, 22, 2); 
    void* result = virtual_malloc (virtual_heap, 12); 
    test_program_break (result);
    add_value_malloc (result, 12);
    void* result2 = virtual_realloc (virtual_heap, result, 10); //2^13 = 8192
    test_program_break (result2);
    check_value_realloc (result2, 10);
    result = virtual_malloc (virtual_heap, 100);
    test_program_break (result);
    add_value_malloc (result, 100);
    result2 = virtual_malloc (virtual_heap, 50);
    test_program_break (result);
    add_value_malloc (result2, 50);
    result = virtual_realloc (virtual_heap, result, 120);
    test_program_break (result);
    check_value_realloc (result, 12);
    result = virtual_realloc (virtual_heap, result2, 1600);
    test_program_break (result);
    check_value_realloc (result, 12);
    virtual_free (virtual_heap, result2);
    result = virtual_malloc (virtual_heap, 1000);
    test_program_break (result);
    add_value_malloc (result, 1024);
    result = virtual_realloc (virtual_heap, result, 5000);
    test_program_break (result);
    check_value_realloc (result, 12);
    result = virtual_realloc (virtual_heap, result, 1600);
    test_program_break (result);
    check_value_realloc (result, 12);
    result = virtual_realloc (virtual_heap, result, 16000);
    test_program_break (result);
    check_value_realloc (result, 9);
    result = virtual_realloc (virtual_heap, result, 100);
    test_program_break (result);
    check_value_realloc (result, 10);
    result = virtual_realloc (virtual_heap, result, 8000);
    test_program_break (result);
    check_value_realloc (result, 10);
    result = virtual_malloc (virtual_heap, 10);
    test_program_break (result);
    add_value_malloc (result, 10);
    result = virtual_malloc (virtual_heap, 500);
    test_program_break (result);
    add_value_malloc (result, 500);
    result = virtual_malloc (virtual_heap, 125);
    test_program_break (result);
    add_value_malloc (result, 125);
    result = virtual_malloc (virtual_heap, 1200);
    test_program_break (result);
    add_value_malloc (result, 1200);
    result = virtual_malloc (virtual_heap, 65);
    test_program_break (result);
    add_value_malloc (result, 65);
    result = virtual_malloc (virtual_heap, 125);
    test_program_break (result);
    add_value_malloc (result, 125);
    void* result3 = virtual_malloc (virtual_heap, 1200);
    test_program_break (result3);
    add_value_malloc (result3, 1200);
    result = virtual_realloc (virtual_heap, result3, 100);
    test_program_break (result3);
    check_value_realloc (result3, 1200);
    result = virtual_malloc (virtual_heap, 5);
    test_program_break (result);
    add_value_malloc (result, 5);
    result = virtual_malloc (virtual_heap, 2);
    test_program_break (result);
    add_value_malloc (result, 2);
    result = virtual_malloc (virtual_heap, 2000);
    test_program_break (result);
    add_value_malloc (result, 2000);
    result = virtual_malloc (virtual_heap, 3000);
    test_program_break (result);
    add_value_malloc (result, 3000);
    
    expected = "allocated 16\nallocated 16\nallocated 8\nallocated 4\nfree 4\nfree 16\nfree 64\nallocated 128\nallocated 128\nallocated 128\nallocated 512\nallocated 128\nallocated 128\nfree 256\nfree 512\nallocated 2048\nallocated 2048\nallocated 2048\nallocated 8192\nallocated 4096\nfree 4096\nfree 8192\nfree 32768\nfree 65536\nfree 131072\nfree 262144\nfree 524288\nfree 1048576\nfree 2097152\n";
    test_virtual_info ();
}

int main() {
    // Your own testing code here
    const struct CMUnitTest tests [] = {
        cmocka_unit_test_setup_teardown (test_double_free, initialise, reset),
        cmocka_unit_test_setup_teardown (test_malloc_simple, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_malloc_multiple, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_free_simple, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_free_multiple, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_init_allocator, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_malloc_not_possible, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_free_not_possible, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_realloc_simple, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_realloc_complex, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_realloc_freed_block, initialise, reset),
    	cmocka_unit_test_setup_teardown (test_realloc_not_possible, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_free_realloced_block, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_increment_decrement, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_zero, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_one, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_initial_size, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_minimum, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_minimum_zero, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_minimum_one, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_malloc_minimum_initial_size, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_one, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_zero, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_initial_size, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_minimum, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_minimum_zero, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_minimum_one, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_realloc_minimum_initial_size, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_integrated, initialise, reset),
   	cmocka_unit_test_setup_teardown (test_integrated_long, initialise, reset)
   	
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
