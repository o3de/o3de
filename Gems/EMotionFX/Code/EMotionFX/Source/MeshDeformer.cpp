/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MeshDeformer.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshDeformer, DeformerAllocator)


    // constructor
    MeshDeformer::MeshDeformer(Mesh* mesh)
        : MCore::RefCounted()
    {
        m_mesh       = mesh;
        m_isEnabled  = true;
    }


    // destructor
    MeshDeformer::~MeshDeformer()
    {
    }


    // check if the deformer is enabled
    bool MeshDeformer::GetIsEnabled() const
    {
        return m_isEnabled;
    }


    // enable or disable it
    void MeshDeformer::SetIsEnabled(bool enabled)
    {
        m_isEnabled = enabled;
    }


    // reinitialize the mesh deformer
    void MeshDeformer::Reinitialize(Actor* actor, Node* node, size_t lodLevel, uint16 highestJointIndex)
    {
        MCORE_UNUSED(actor);
        MCORE_UNUSED(node);
        MCORE_UNUSED(lodLevel);
        MCORE_UNUSED(highestJointIndex);
    }
} // namespace EMotionFX
