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
