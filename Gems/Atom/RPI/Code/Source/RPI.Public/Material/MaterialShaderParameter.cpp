/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/MaterialShaderParameter.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    namespace RPI
    {

        MaterialShaderParameter::MaterialShaderParameter(const MaterialShaderParameterLayout* layout)
            : m_layout(layout)
        {
            if (!m_layout->GetDescriptors().empty())
            {
                auto last{ m_layout->GetDescriptors().back().m_structuredBufferBinding };
                const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
                for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                {
                    m_structuredBufferData[deviceIndex].resize(last.m_offset + last.m_elementSize * last.m_elementCount, 0);
                }
            }
            else
            {
                AZ_Assert(false, "MaterialShaderParameter needs a layout");
            }
        }

        void MaterialShaderParameter::SetStructuredBufferData(
            const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& data)
        {
            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                SetStructuredBufferData(desc, data, deviceIndex);
            }
        }

        void MaterialShaderParameter::SetStructuredBufferData(
            const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& deviceData, const int deviceIndex)
        {
            auto& binding{ desc->m_structuredBufferBinding };

            AZ_Assert(
                binding.m_elementSize * binding.m_elementCount == deviceData.size_bytes(),
                "Size mismatch when setting the Material Shader Parameter data for %s %s: expected: %lld bytes, provided %lld bytes",
                desc->m_typeName.c_str(),
                desc->m_name.c_str(),
                binding.m_elementSize * binding.m_elementCount,
                deviceData.size_bytes());

            size_t minBufferSize = binding.m_offset + binding.m_elementSize * binding.m_elementCount;
            if (m_structuredBufferData[deviceIndex].size() < minBufferSize)
            {
                m_structuredBufferData[deviceIndex].resize(minBufferSize, 0);
            }
            for (int byteIndex = 0; byteIndex < deviceData.size(); ++byteIndex)
            {
                m_structuredBufferData[deviceIndex][binding.m_offset + byteIndex] = deviceData[byteIndex];
            }
        }

    } // namespace RPI
} // namespace AZ
