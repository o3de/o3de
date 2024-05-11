/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include headers
#include "VertexAttributeLayer.h"
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(VertexAttributeLayer, MeshAllocator)


    // constructor
    VertexAttributeLayer::VertexAttributeLayer(uint32 numAttributes, bool keepOriginals)
        : MCore::RefCounted()
    {
        m_numAttributes  = numAttributes;
        m_keepOriginals  = keepOriginals;
        m_nameId         = MCore::GetStringIdPool().GenerateIdForString("");
    }


    // destructor
    VertexAttributeLayer::~VertexAttributeLayer()
    {
    }


    // returns true when we deal with the abstract vertex attribute layer class
    // not very elegant to do it this way, but it's probably the best way with this design
    bool VertexAttributeLayer::GetIsAbstractDataClass() const
    {
        return false;
    }


    // set the name
    void VertexAttributeLayer::SetName(const char* name)
    {
        m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // get the name
    const char* VertexAttributeLayer::GetName() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId).c_str();
    }


    // get the name string
    const AZStd::string& VertexAttributeLayer::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId);
    }


    // get the name ID
    uint32 VertexAttributeLayer::GetNameID() const
    {
        return m_nameId;
    }
} // namespace EMotionFX
