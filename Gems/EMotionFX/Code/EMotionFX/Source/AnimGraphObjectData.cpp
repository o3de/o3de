/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNode.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphObjectData, AnimGraphObjectDataAllocator)

    AnimGraphObjectData::AnimGraphObjectData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : MCore::RefCounted()
    {
        m_object             = object;
        m_animGraphInstance = animGraphInstance;
        m_objectFlags        = 0;
    }

    AnimGraphObjectData::~AnimGraphObjectData()
    {
    }

    uint32 AnimGraphObjectData::Save(uint8* outputBuffer) const
    {
        if (outputBuffer)
        {
            MCore::MemCopy(outputBuffer, (uint8*)this, sizeof(*this));
        }

        return sizeof(*this);
    }

    // load from a given data buffer
    uint32 AnimGraphObjectData::Load(const uint8* dataBuffer)
    {
        if (dataBuffer)
        {
            MCore::MemCopy((uint8*)this, dataBuffer, sizeof(*this));
        }

        return sizeof(*this);
    }

    void AnimGraphObjectData::SaveChunk(const uint8* chunkData, uint32 chunkSize, uint8** inOutBuffer, uint32& inOutSize) const
    {
        if (*inOutBuffer)
        {
            memcpy(*inOutBuffer, chunkData, chunkSize);
            *inOutBuffer += chunkSize;
        }
        inOutSize += chunkSize;
    }

    void AnimGraphObjectData::LoadChunk(uint8* chunkData, uint32 chunkSize, uint8** inOutBuffer, uint32& inOutSize)
    {
        if (*inOutBuffer)
        {
            memcpy(chunkData, *inOutBuffer, chunkSize);
            *inOutBuffer += chunkSize;
        }
        inOutSize += chunkSize;
    }
} // namespace EMotionFX
