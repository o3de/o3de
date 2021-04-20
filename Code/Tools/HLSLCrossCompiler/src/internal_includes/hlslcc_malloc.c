// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4232, "-Wunknown-warning-option") // address of malloc/free/calloc/realloc are not static
void* (*hlslcc_malloc)(size_t size) = malloc;
void* (*hlslcc_calloc)(size_t num,size_t size) = calloc;
void (*hlslcc_free)(void *p) = free;
void* (*hlslcc_realloc)(void *p,size_t size) = realloc;
AZ_POP_DISABLE_WARNING
