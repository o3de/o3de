/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Descriptor.h>

namespace AZ
{
    namespace DX12
    {
        DescriptorHandle::DescriptorHandle()
            : m_index{ NullIndex }
            , m_type{ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES }
            , m_flags{ D3D12_DESCRIPTOR_HEAP_FLAG_NONE }
        {}

        DescriptorHandle::DescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t index)
            : m_index{ index }
            , m_type{ type }
            , m_flags{ flags }
        {}

        bool DescriptorHandle::IsNull() const
        {
            return m_type == D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        }

        bool DescriptorHandle::IsShaderVisible() const
        {
            return m_flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }

        DescriptorHandle DescriptorHandle::operator+ (uint32_t offset) const
        {
            return DescriptorHandle(m_type, m_flags, m_index + offset);
        }

        DescriptorHandle& DescriptorHandle::operator+= (uint32_t offset)
        {
            m_index += offset;
            return *this;
        }

        DescriptorTable::DescriptorTable(DescriptorHandle handle, uint32_t count)
            : m_offset(handle)
            , m_size(count)
        {}

        DescriptorHandle DescriptorTable::operator [] (uint32_t i) const
        {
            return m_offset + i;
        }

        DescriptorHandle DescriptorTable::GetOffset() const
        {
            return m_offset;
        }

        D3D12_DESCRIPTOR_HEAP_TYPE DescriptorTable::GetType() const
        {
            return m_offset.m_type;
        }

        D3D12_DESCRIPTOR_HEAP_FLAGS DescriptorTable::GetFlags() const
        {
            return m_offset.m_flags;
        }

        uint32_t DescriptorTable::GetSize() const
        {
            return m_size;
        }

        bool DescriptorTable::IsNull() const
        {
            return m_offset.IsNull() || !m_size;
        }

        bool DescriptorTable::IsValid() const
        {
            return !IsNull();
        }
    }
}
