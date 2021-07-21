/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/MemoryAllocation.h>
#include <RHI/Memory.h>

namespace AZ
{
    namespace Vulkan
    {
        enum class MemoryAllocationType : uint32_t
        {
            Unique,
            SubAllocated
        };

        //! Represents a view into GPU memory object. It contains a smart pointer to the Memory Object.
        template<typename T>
        class MemoryTypeView
        {
            using Allocation = RHI::MemoryAllocation<T>;

        public:
            MemoryTypeView() = default;
            MemoryTypeView(const Allocation& memAllocation, MemoryAllocationType memoryType);
            MemoryTypeView(RHI::Ptr<T> memory, size_t offset, size_t size, size_t alignment, MemoryAllocationType memoryType);

            ~MemoryTypeView() = default;

            //! Returns if the MemoryView has a valid value.
            bool IsValid() const;

            //! Returns the offset relative to the base memory address in bytes.
            size_t GetOffset() const;

            //! Returns the size of the memory view region in bytes.
            size_t GetSize() const;

            //! Returns the alignment of the memory view region in bytes.
            size_t GetAlignment() const;

            //! A convenience method to map the resource region spanned by the view for CPU access.
            CpuVirtualAddress Map(RHI::HostMemoryAccess hostAccess);

            //! A convenience method for unmapping the resource region spanned by the view.
            void Unmap(RHI::HostMemoryAccess hostAccess);

            //! Returns the allocation type.
            MemoryAllocationType GetAllocationType() const;

            //! Sets the name of the underlying memory object. It only applies the name if the allocation is Unique.
            void SetName(const AZStd::string_view& name) const;

            //! Returns the allocation that the View represents.
            const Allocation& GetAllocation() const;

        private:
            MemoryAllocationType m_allocationType = MemoryAllocationType::Unique;
            Allocation m_memoryAllocation;
        };

        template<typename T>
        MemoryTypeView<T>::MemoryTypeView(const Allocation& memAllocation, MemoryAllocationType memoryType)
            : m_memoryAllocation(memAllocation)
            , m_allocationType(memoryType)
        {
        }

        template<typename T>
        MemoryTypeView<T>::MemoryTypeView(RHI::Ptr<T> memory, size_t offset, size_t size, size_t alignment, MemoryAllocationType memoryType)
            : MemoryTypeView(Allocation(memory, offset, size, alignment), memoryType)
        {
        }

        template<typename T>
        bool MemoryTypeView<T>::IsValid() const
        {
            return m_memoryAllocation.m_memory != nullptr;
        }

        template<typename T>
        size_t MemoryTypeView<T>::GetOffset() const
        {
            return m_memoryAllocation.m_offset;
        }

        template<typename T>
        size_t MemoryTypeView<T>::GetSize() const
        {
            return m_memoryAllocation.m_size;
        }

        template<typename T>
        size_t MemoryTypeView<T>::GetAlignment() const
        {
            return m_memoryAllocation.m_alignment;
        }

        template<typename T>
        CpuVirtualAddress MemoryTypeView<T>::Map(RHI::HostMemoryAccess hostAccess)
        {
            return m_memoryAllocation.m_memory->Map(m_memoryAllocation.m_offset, m_memoryAllocation.m_size, hostAccess);
        }

        template<typename T>
        void MemoryTypeView<T>::Unmap(RHI::HostMemoryAccess hostAccess)
        {
            m_memoryAllocation.m_memory->Unmap(m_memoryAllocation.m_offset, hostAccess);
        }

        template<typename T>
        MemoryAllocationType MemoryTypeView<T>::GetAllocationType() const
        {
            return m_allocationType;
        }

        template<typename T>
        void MemoryTypeView<T>::SetName(const AZStd::string_view& name) const
        {
            if (IsValid() && m_allocationType == MemoryAllocationType::Unique)
            {
                m_memoryAllocation.m_memory->SetName(Name{ name });
            }
        }

        template<typename T>
        const typename MemoryTypeView<T>::Allocation& MemoryTypeView<T>::GetAllocation() const
        {
            return m_memoryAllocation;
        }
    }
}
