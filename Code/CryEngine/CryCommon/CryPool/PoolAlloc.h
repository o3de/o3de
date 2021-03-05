/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#if defined(POOLALLOCTESTSUIT)
//cheat just for unit testing on windows
#include "BaseTypes.h"
#define ILINE           inline
#endif

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryPool/PoolAlloc_h)
#elif defined(APPLE) || defined(LINUX)
#define POOLALLOC_H_TRAIT_USE_MEMALIGN 1
#endif


#if POOLALLOC_H_TRAIT_USE_MEMALIGN
#define CPA_ALLOC                           memalign
#define CPA_FREE                            free
#else
#define CPA_ALLOC                           _aligned_malloc
#define CPA_FREE                            _aligned_free
#endif
#define CPA_ASSERT                      assert
#define CPA_ASSERT_STATIC(X)    {uint8 assertdata[(X) ? 0 : 1]; }
#define CPA_BREAK                           __debugbreak()

#include "List.h"
#include "Memory.h"
#include "Container.h"
#include "Allocator.h"
#include "Defrag.h"
#include "STLWrapper.h"
#include "Inspector.h"
#include "Fallback.h"
#if !defined(POOLALLOCTESTSUIT)
#include "ThreadSafe.h"
#endif

#undef CPA_ASSERT
#undef CPA_ASSERT_STATIC
#undef CPA_BREAK
