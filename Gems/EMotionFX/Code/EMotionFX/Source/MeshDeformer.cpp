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

// include the required headers
#include "MeshDeformer.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshDeformer, DeformerAllocator, 0)


    // constructor
    MeshDeformer::MeshDeformer(Mesh* mesh)
        : BaseObject()
    {
        mMesh       = mesh;
        mIsEnabled  = true;
    }


    // destructor
    MeshDeformer::~MeshDeformer()
    {
    }


    // check if the deformer is enabled
    bool MeshDeformer::GetIsEnabled() const
    {
        return mIsEnabled;
    }


    // enable or disable it
    void MeshDeformer::SetIsEnabled(bool enabled)
    {
        mIsEnabled = enabled;
    }


    // reinitialize the mesh deformer
    void MeshDeformer::Reinitialize(Actor* actor, Node* node, uint32 lodLevel)
    {
        MCORE_UNUSED(actor);
        MCORE_UNUSED(node);
        MCORE_UNUSED(lodLevel);
    }
} // namespace EMotionFX
