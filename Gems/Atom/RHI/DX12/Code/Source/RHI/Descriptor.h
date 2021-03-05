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
#pragma once

#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        struct DescriptorHandle
        {
            static const uint32_t NullIndex = (uint32_t)-1;

            DescriptorHandle();
            DescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t index);

            bool IsNull() const;
            bool IsShaderVisible() const;

            DescriptorHandle operator+ (uint32_t offset) const;
            DescriptorHandle& operator+= (uint32_t offset);

            uint32_t m_index : 32;
            D3D12_DESCRIPTOR_HEAP_TYPE m_type : 4;
            D3D12_DESCRIPTOR_HEAP_FLAGS m_flags : 4;
        };

        class DescriptorTable
        {
        public:
            DescriptorTable() = default;
            DescriptorTable(DescriptorHandle handle, uint16_t count);

            DescriptorHandle operator [] (uint32_t i) const;

            DescriptorHandle GetOffset() const;
            D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
            D3D12_DESCRIPTOR_HEAP_FLAGS GetFlags() const;
            uint16_t GetSize() const;

            bool IsNull() const;
            bool IsValid() const;

        private:
            DescriptorHandle m_offset;
            uint16_t m_size = 0;
        };
    }
}