/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {

        // This helper class manages a re-sizable structured or typed buffer which is (only) used for a shader's SRV
        class GpuBufferHandler
        {
        public:

            struct Descriptor
            {
                AZStd::string m_bufferName; // Name of the buffer itself
                AZStd::string m_bufferSrgName; // Name of the buffer in the SRG
                AZStd::string m_elementCountSrgName; // Name of the constant for buffer size in the SRG
                const RHI::ShaderResourceGroupLayout* m_srgLayout = nullptr; // The srg to query for the buffer name and count.
                uint32_t m_elementSize = 1; // The size of the elements stored in the buffer.
                RHI::Format m_elementFormat = RHI::Format::Unknown; // Type of the elements (if typed)
            };

            GpuBufferHandler() = default;
            GpuBufferHandler(const Descriptor& descriptor);

            template <typename T>
            bool UpdateBuffer(const T* data, uint32_t elementCount);
            template <typename T>
            bool UpdateBuffer(const AZStd::vector<T>& data);
            bool UpdateBuffer(const AZStd::unordered_map<int, const void*>& data, uint32_t elementCount);

            void UpdateSrg(RPI::ShaderResourceGroup* srg) const;

            bool IsValid() const;
            void Release();

            uint32_t GetElementCount()const { return m_elementCount; }
            const Data::Instance<RPI::Buffer> GetBuffer()const { return m_buffer; }

        private:

            bool UpdateBuffer(uint32_t elementCount, const void* data);

            Data::Instance<RPI::Buffer> m_buffer;
            RHI::ShaderInputBufferIndex m_bufferIndex;
            RHI::ShaderInputConstantIndex m_elementCountIndex;
            uint32_t m_elementCount = 0;
            uint32_t m_elementSize = 0;
        };
        template <typename T>
        bool GpuBufferHandler::UpdateBuffer(const T* data, uint32_t elementCount)
        {
            AZ_Assert(sizeof(T) == m_elementSize, "Size of templated type doesn't match the size this GpuBuffer was initialized with.");
            return UpdateBuffer(elementCount, data);
        }

        template <typename T>
        bool GpuBufferHandler::UpdateBuffer(const AZStd::vector<T>& data)
        {
            AZ_Assert(sizeof(T) == m_elementSize, "Size of templated type doesn't match the size this GpuBuffer was initialized with.");
            return UpdateBuffer(aznumeric_cast<uint32_t>(data.size()), data.data());
        }
    } // namespace Render
} // namespace AZ
