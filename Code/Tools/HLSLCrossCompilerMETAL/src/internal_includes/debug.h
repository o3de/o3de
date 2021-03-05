// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef _DEBUG
#include "assert.h"
#define ASSERT(expr) CustomAssert(expr)
static void CustomAssert(int expression)
{
    if(!expression)
    {
        assert(0);
    }
}
#else
#define ASSERT(expr)
#endif

#endif
