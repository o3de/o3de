// Modifications copyright Amazon.com, Inc. or its affiliates

#include "structsMetal.h"

int IsAtomicVar(const ShaderVarType* const var, AtomicVarList* const list)
{
    for (uint32_t i = 0; i < list->Filled; i++)
    {
        if (var == list->AtomicVars[i])
        {
            return 1;
        }
    }
    return 0;
}
