// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef STRUCTSS_METAL_H
#define STRUCTSS_METAL_H

#include "hlslcc.h"
#include <stdint.h>

typedef struct AtomicVarList_s
{
    const ShaderVarType**  AtomicVars;
    uint32_t    Filled;
    uint32_t    Size;
} AtomicVarList;

int IsAtomicVar(const ShaderVarType* const var, AtomicVarList* const list);


#endif
