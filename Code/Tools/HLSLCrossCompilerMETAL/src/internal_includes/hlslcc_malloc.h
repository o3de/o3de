// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef __HLSCC_MALLOC_H
#define __HLSCC_MALLOC_H

extern void* (*hlslcc_malloc)(size_t size);
extern void* (*hlslcc_calloc)(size_t num,size_t size);
extern void (*hlslcc_free)(void *p);
extern void* (*hlslcc_realloc)(void *p,size_t size);

#define bstr__alloc hlslcc_malloc
#define bstr__free hlslcc_free
#define bstr__realloc hlslcc_realloc
#endif