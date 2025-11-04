/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/PlatformIncl.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include <AzCore/Memory/Memory.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeColor.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraphInstance;
    class AnimGraphNode;
    class AnimGraphPose;

    // implement standard load and save
#define EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE                      \
public:                                                                  \
    uint32 Save(uint8 * outputBuffer) const override             \
    {                                                                    \
        if (outputBuffer) {                                              \
            MCore::MemCopy(outputBuffer, (uint8*)this, sizeof(*this)); } \
        return sizeof(*this);                                            \
    }                                                                    \
                                                                         \
    uint32 Load(const uint8 * dataBuffer) override               \
    {                                                                    \
        if (dataBuffer) {                                                \
            MCore::MemCopy((uint8*)this, dataBuffer, sizeof(*this)); }   \
        return sizeof(*this);                                            \
    }


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphObjectData
        : public MCore::RefCounted
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            FLAGS_HAS_ERROR = 1 << 0
        };

        AnimGraphObjectData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
        virtual ~AnimGraphObjectData();

        MCORE_INLINE AnimGraphObject* GetObject() const                { return m_object; }
        void SetObject(AnimGraphObject* object)                        { m_object = object; }

        // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
        virtual uint32 Save(uint8* outputBuffer) const;
        void SaveChunk(const uint8* chunkData, uint32 chunkSize, uint8** inOutBuffer, uint32& inOutSize) const;
        template <class T>
        void SaveVectorOfObjects(const AZStd::vector<T>& objects, uint8** inOutBuffer, uint32& inOutSize) const;

        virtual uint32 Load(const uint8* dataBuffer);
        void LoadChunk(uint8* chunkData, uint32 chunkSize, uint8** inOutBuffer, uint32& inOutSize);
        template <class T>
        void LoadVectorOfObjects(AZStd::vector<T>& inOutObjects, uint8** inOutBuffer, uint32& inOutSize);

        virtual void Reset() {}
        virtual void Update() {}

        void Invalidate() { m_invalidated = true; }
        bool IsInvalidated() const { return m_invalidated; }
        void Validate() { m_invalidated = false; }

        MCORE_INLINE uint8 GetObjectFlags() const                       { return m_objectFlags; }
        MCORE_INLINE void SetObjectFlags(uint8 flags)                   { m_objectFlags = flags; }
        MCORE_INLINE void EnableObjectFlags(uint8 flagsToEnable)        { m_objectFlags |= flagsToEnable; }
        MCORE_INLINE void DisableObjectFlags(uint8 flagsToDisable)      { m_objectFlags &= ~flagsToDisable; }
        MCORE_INLINE void SetObjectFlags(uint8 flags, bool enabled)
        {
            if (enabled)
            {
                m_objectFlags |= flags;
            }
            else
            {
                m_objectFlags &= ~flags;
            }
        }
        MCORE_INLINE bool GetIsObjectFlagEnabled(uint8 flag) const      { return (m_objectFlags & flag) != 0; }

        MCORE_INLINE bool GetHasError() const                           { return (m_objectFlags & FLAGS_HAS_ERROR); }
        MCORE_INLINE void SetHasError(bool hasError)                    { SetObjectFlags(FLAGS_HAS_ERROR, hasError); }

        AnimGraphInstance* GetAnimGraphInstance() { return m_animGraphInstance; }
        const AnimGraphInstance* GetAnimGraphInstance() const { return m_animGraphInstance; }

    protected:
        AnimGraphObject*    m_object;               /**< Pointer to the object where this data belongs to. */
        AnimGraphInstance*  m_animGraphInstance;    /**< The animgraph instance where this unique data belongs to. */
        uint8               m_objectFlags;
        bool m_invalidated = true;
    };

    template <class T>
    void AnimGraphObjectData::SaveVectorOfObjects(const AZStd::vector<T>& objects, uint8** inOutBuffer, uint32& inOutSize) const
    {
        const size_t numObjects = objects.size();
        SaveChunk((uint8*)&numObjects, sizeof(size_t), inOutBuffer, inOutSize);
        if (numObjects > 0)
        {
            const uint32 sizeInBytes = static_cast<uint32>(numObjects * sizeof(T));
            SaveChunk((uint8*)&objects[0], sizeInBytes, inOutBuffer, inOutSize);
        }
    }

    template <class T>
    void AnimGraphObjectData::LoadVectorOfObjects(AZStd::vector<T>& inOutObjects, uint8** inOutBuffer, uint32& inOutSize)
    {
        size_t numObjects;
        LoadChunk((uint8*)&numObjects, sizeof(size_t), inOutBuffer, inOutSize);

        if (*inOutBuffer)
        {
            inOutObjects.resize(numObjects);
            if (numObjects > 0)
            {
                const uint32 sizeInBytes = static_cast<uint32>(numObjects * sizeof(T));
                LoadChunk((uint8*)&inOutObjects[0], sizeInBytes, inOutBuffer, inOutSize);
            }
        }
    }
} // namespace EMotionFX
