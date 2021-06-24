/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Allocators.h"

namespace EMStudio
{
    UIAllocator::UIAllocator() 
        : UIAllocator::Base("UIAllocator", "EMotion FX UI memory allocator")
    {
    }
}
