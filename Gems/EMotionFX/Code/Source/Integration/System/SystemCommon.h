/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <MCore/Source/Vector.h>
#include <MCore/Source/MemoryObject.h>

#include <EMotionFX/Source/Transform.h>

namespace AZStd
{
    /**
     * Intrusive ptr for EMotionFX-owned objects (uses EMotionFX's internal ref-counting MCore::Destroy()).
     */
    template<>
    struct IntrusivePtrCountPolicy<MCore::MemoryObject>
    {
        static AZ_FORCE_INLINE void add_ref(MCore::MemoryObject* ptr) 
        { 
            ptr->IncreaseReferenceCount();
        }
        static AZ_FORCE_INLINE void release(MCore::MemoryObject* ptr) 
        { 
            MCore::Destroy(ptr); // Calls DecreaseReferenceCount.
        }
    };
}

namespace EMotionFX
{
    namespace Integration
    {
        /**
         * System allocator to be used for all EMotionFX and EMotionFXAnimation gem persistent allocations.
         */
        class EMotionFXAllocator
            : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
        {
        public:
            using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;

            AZ_RTTI(EMotionFXAllocator, "{00AEC34F-4A00-4ECB-BC9C-7221E76337D6}", Base);
        };

        /**
         * Intrusive ptr for EMotionFX-owned objects.
         * Uses EMotionFX's internal ref-counting.
         */
        template <typename ObjectType>
        class EMotionFXPtr
        {
        public:

            /// Use only to initialize a new EMotionFXPtr<> given an EMotionFX SDK object not currently owned by an EMotionFXPtr<>.
            /// This is generally only appropriate for use when an EMotionFX object has just been constructed.
            static EMotionFXPtr<ObjectType> MakeFromNew(ObjectType* object)
            {
                AZ_Assert(object, "CreateFromNew called with invalid object.");
                AZ_Assert(object && object->GetReferenceCount() == 1, "Newly constructed EMotion FX objects are expected to have a referene count initialized to 1.");

                // EMotionFX initializes objects with a ref count already at 1. So for newly-constructed objects that we're
                // managing through smart pointers, it's not necessary to increment ref count during initial acquisition.
                EMotionFXPtr<ObjectType> ptr;
                ptr.m_ptr = object;
                return ptr;
            }

            AZ_FORCE_INLINE explicit EMotionFXPtr(ObjectType* object = nullptr)
            {
                *this = object;
            }

            AZ_FORCE_INLINE EMotionFXPtr(const EMotionFXPtr<ObjectType>& rhs)
            {
                *this = rhs;
            }

            AZ_FORCE_INLINE ~EMotionFXPtr()
            {
                if (m_ptr)
                {
                    MCore::Destroy(m_ptr); // Calls DecreaseReferenceCount.
                }
            }

            AZ_FORCE_INLINE void reset(ObjectType* object = nullptr)
            {
                *this = object;
            }

            AZ_FORCE_INLINE void operator=(ObjectType* object)
            {
                if (m_ptr)
                {
                    MCore::Destroy(m_ptr); // Calls DecreaseReferenceCount.
                    m_ptr = nullptr;
                }

                m_ptr = object;

                if (m_ptr)
                {
                    m_ptr->IncreaseReferenceCount();
                }
            }

            AZ_FORCE_INLINE void operator=(const EMotionFXPtr<ObjectType>& rhs)
            {
                reset(rhs ? rhs.get() : nullptr);
            }

            AZ_FORCE_INLINE ObjectType* operator ->() const
            {
                AZ_Assert(m_ptr, "Attempting to dereference a null EMotion FX object pointer.");
                return m_ptr;
            }

            AZ_FORCE_INLINE ObjectType* get() const
            {
                return m_ptr;
            }

            AZ_FORCE_INLINE operator bool() const
            {
                return m_ptr != nullptr;
            }

            AZ_FORCE_INLINE bool operator==(const EMotionFXPtr<ObjectType>& rhs) const
            {
                return (m_ptr == rhs.m_ptr);
            }

            AZ_FORCE_INLINE bool operator==(const ObjectType* rhs) const
            {
                return (m_ptr == rhs);
            }

            AZ_FORCE_INLINE bool operator!=(const EMotionFXPtr<ObjectType>& rhs) const
            {
                return !(m_ptr == rhs.m_ptr);
            }

            AZ_FORCE_INLINE bool operator!=(const ObjectType* rhs) const
            {
                return !(m_ptr == rhs);
            }

        private:

            ObjectType* m_ptr = nullptr;
        };

        /**
         * EMotionFX memory hooks
         */
        AZ_FORCE_INLINE void* EMotionFXAlloc(size_t numBytes, AZ::u16 categoryID, AZ::u16 blockID, const char* filename, AZ::u32 lineNr)
        {
            (void)categoryID;
            (void)blockID;
            (void)lineNr;
            return AZ::AllocatorInstance<EMotionFXAllocator>::Get().Allocate(numBytes, 8, 0, "EMotionFX", filename, lineNr);
        }

        AZ_FORCE_INLINE void* EMotionFXRealloc(void* memory, size_t numBytes, AZ::u16 categoryID, AZ::u16 blockID, const char* filename, AZ::u32 lineNr)
        {
            (void)categoryID;
            (void)blockID;
            (void)filename;
            (void)lineNr;
            return AZ::AllocatorInstance<EMotionFXAllocator>::Get().ReAllocate(memory, numBytes, 8);
        }

        AZ_FORCE_INLINE void EMotionFXFree(void* memory)
        {
            AZ::AllocatorInstance<EMotionFXAllocator>::Get().DeAllocate(memory);
        }

    } //namespace Integration
} // namespace EMotionFX
