/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Vulkan.h>
#include <RHI/SpecializationConstantData.h>

namespace AZ::Vulkan
{
    template<class T>
    void AddSpecializationValue(AZStd::vector<uint8_t>& data, const RHI::SpecializationValue& specValue)
    {
        size_t size = sizeof(T);
        size_t offset = data.size();
        data.resize(data.size() + size);
        T value = static_cast<T>(specValue.GetIndex());
        memcpy(data.data() + offset, &value, size);
    }

    RHI::ResultCode SpecializationConstantData::Init(const RHI::PipelineStateDescriptor& pipelineDescriptor)
    {
        m_specializationData.reserve(pipelineDescriptor.m_specializationData.size() * sizeof(uint32_t));
        for (const RHI::SpecializationConstant& specialization : pipelineDescriptor.m_specializationData)
        {
            m_specializationMap.emplace_back();
            VkSpecializationMapEntry& entry = m_specializationMap.back();
            entry.constantID = specialization.m_id;
            entry.offset = aznumeric_cast<uint32_t>(m_specializationData.size());
            switch (specialization.m_type)
            {
            case RHI::SpecializationType::Integer:
                entry.size = sizeof(uint32_t);
                AddSpecializationValue<uint32_t>(m_specializationData, specialization.m_value);
                break;
            case RHI::SpecializationType::Bool:
                entry.size = sizeof(VkBool32);
                AddSpecializationValue<VkBool32>(m_specializationData, specialization.m_value);
                break;
            default:
                AZ_Assert(false, "Invalid specialization type %d", specialization.m_type);
                return RHI::ResultCode::InvalidArgument;
            }
        }

        m_specializationInfo.dataSize = m_specializationData.size();
        m_specializationInfo.mapEntryCount = aznumeric_cast<uint32_t>(m_specializationMap.size());
        m_specializationInfo.pData = m_specializationData.data();
        m_specializationInfo.pMapEntries = m_specializationMap.data();
        return RHI::ResultCode::Success;
    }

    void SpecializationConstantData::Shutdown()
    {
        m_specializationInfo = {};
        m_specializationMap.clear();
        m_specializationData.clear();
    }

    const VkSpecializationInfo* SpecializationConstantData::GetVkSpecializationInfo() const
    {
        return m_specializationInfo.mapEntryCount ? &m_specializationInfo : nullptr;
    }
} // namespace AZ::Vulkan
