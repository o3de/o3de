/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "BaseObject.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    // the constructor
    BaseObject::BaseObject()
        : MCore::MemoryObject()
    {
    }


    // the destructor
    BaseObject::~BaseObject()
    {
    }


    // delete this object
    void BaseObject::Delete()
    {
        delete this;
    }
}   // namespace EMotionFX
