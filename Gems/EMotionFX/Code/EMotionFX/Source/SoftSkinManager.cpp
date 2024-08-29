/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "SoftSkinManager.h"
#include "SoftSkinDeformer.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SoftSkinManager, SoftSkinManagerAllocator)


    // constructor
    SoftSkinManager::SoftSkinManager()
        : MCore::RefCounted()
    {
    }


    // create
    SoftSkinManager* SoftSkinManager::Create()
    {
        return aznew SoftSkinManager();
    }


    // create the deformer
    SoftSkinDeformer* SoftSkinManager::CreateDeformer(Mesh* mesh)
    {
        return SoftSkinDeformer::Create(mesh);
    }
} // namespace EMotionFX
