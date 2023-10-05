/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI/DeviceObject.h>
#include <RHI/VulkanMemoryAllocation.h>

namespace AZ
{
    namespace Vulkan
    {
        enum class MemoryAllocationType : uint32_t
        {
            Unique,
            SubAllocated
        };

        //! Represents a view into GPU memory object. It contains a smart pointer to the allocation that it's "viewing".
        template<typename T>
        class MemoryTypeView
        {
        public:
            MemoryTypeView() = default;
            MemoryTypeView(RHI::Ptr<T> alloc);
            MemoryTypeView(RHI::Ptr<T> alloc, size_t offset, size_t size);

            ~MemoryTypeView() = default;

            //! Returns if the MemoryView has a valid value.
            bool IsValid() const;

            //! Returns the offset relative to the memory allocation in bytes.
            size_t GetOffset() const;

            //! Returns the size of the memory view region in bytes.
            size_t GetSize() const;

            //! A convenience method to map the resource region spanned by the view for CPU access.
            CpuVirtualAddress Map(RHI::HostMemoryAccess hostAccess);

            //! A convenience method for unmapping the resource region spanned by the view.
            void Unmap(RHI::HostMemoryAccess hostAccess);

            //! Returns the allocation type.
            MemoryAllocationType GetAllocationType() const;

            //! Sets the name of the underlying memory object. It only applies the name if the allocation is Unique.
            void SetName(const AZStd::string_view& name) const;

            //! Returns the allocation that the View represents.
            RHI::Ptr<T> GetAllocation() const;

        private:
            MemoryTypeView(RHI::Ptr<T> alloc, size_t offset, size_t size, MemoryAllocationType type);

            MemoryAllocationType m_allocationType = MemoryAllocationType::Unique;
            RHI::Ptr<T> m_allocation;
            size_t m_offset = 0;
            size_t m_size = 0;
        };

        template<typename T>
        MemoryTypeView<T>::MemoryTypeView(RHI::Ptr<T> alloc, size_t offset, size_t size)
            : MemoryTypeView(alloc, offset, size, MemoryAllocationType::SubAllocated)
        {
        }

        template<typename T>
        MemoryTypeView<T>::MemoryTypeView(RHI::Ptr<T> alloc)
            : MemoryTypeView(alloc, 0, alloc->GetSize(), MemoryAllocationType::Unique)
        {
        }

        template<typename T>
        MemoryTypeView<T>::MemoryTypeView(RHI::Ptr<T> alloc, size_t offset, size_t size, MemoryAllocationType type)
            : m_allocation(alloc)
            , m_offset(offset)
            , m_size(size)
            , m_allocationType(type)
        {
        }

        template<typename T>
        bool MemoryTypeView<T>::IsValid() const
        {
            return m_allocation != nullptr;
        }

        template<typename T>
        size_t MemoryTypeView<T>::GetOffset() const
        {
            return m_offset;
        }

        template<typename T>
        size_t MemoryTypeView<T>::GetSize() const
        {
            return m_size;
        }

        template<typename T>
        CpuVirtualAddress MemoryTypeView<T>::Map(RHI::HostMemoryAccess hostAccess)
        {
            return m_allocation->Map(m_offset, m_size, hostAccess);
        }

        template<typename T>
        void MemoryTypeView<T>::Unmap(RHI::HostMemoryAccess hostAccess)
        {
            m_allocation->Unmap(m_offset, hostAccess);
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
                m_allocation->SetName(Name{ name });
            }
        }

        template<typename T>
        RHI::Ptr<T> MemoryTypeView<T>::GetAllocation() const
        {
            return m_allocation;
        }
    }
}
