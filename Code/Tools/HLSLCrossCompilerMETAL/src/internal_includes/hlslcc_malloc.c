// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include <stdlib.h>

#ifdef __APPLE_CC__
    #include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

// Wrapping these functions since we are taking the address of them and the std functions are dllimport which produce
// warning C4232
void* std_malloc(size_t size)
{
    return malloc(size);
}

void* std_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

void std_free(void* p)
{
    free(p);
}

void* std_realloc(void* p, size_t size)
{
    return realloc(p, size);
}

void* (*hlslcc_malloc)(size_t size) = std_malloc;
void* (*hlslcc_calloc)(size_t num,size_t size) = std_calloc;
void (*hlslcc_free)(void *p) = std_free;
void* (*hlslcc_realloc)(void *p,size_t size) = std_realloc;
