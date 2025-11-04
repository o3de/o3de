/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImageView.h>

namespace AZ::RHI
{
    void DeviceShaderResourceGroup::Compile(const DeviceShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
    {
        switch (compileMode)
        {
        case CompileMode::Async:
            GetPool()->QueueForCompile(*this, groupData);
            break;
        case CompileMode::Sync:
            GetPool()->Compile(*this, groupData);
            break;
        default:
            AZ_Assert(false, "Invalid SRG Compile mode %d", compileMode);
            break;
        }            
    }

    uint32_t DeviceShaderResourceGroup::GetBindingSlot() const
    {
        return m_bindingSlot;
    }

    bool DeviceShaderResourceGroup::IsQueuedForCompile() const
    {
        return m_isQueuedForCompile;
    }

    const DeviceShaderResourceGroupPool* DeviceShaderResourceGroup::GetPool() const
    {
        return static_cast<const DeviceShaderResourceGroupPool*>(DeviceResource::GetPool());
    }

    DeviceShaderResourceGroupPool* DeviceShaderResourceGroup::GetPool()
    {
        return static_cast<DeviceShaderResourceGroupPool*>(DeviceResource::GetPool());
    }

    const DeviceShaderResourceGroupData& DeviceShaderResourceGroup::GetData() const
    {
        return m_data;
    }

    void DeviceShaderResourceGroup::SetData(const DeviceShaderResourceGroupData& data)
    {
        m_data = data;
        uint32_t sourceUpdateMask = data.GetUpdateMask();
            
        //RHI has it's own copy of update mask that is reset after Compile is called m_updateMaskResetLatency times.
        m_rhiUpdateMask |= sourceUpdateMask;
        for (uint32_t i = 0; i < static_cast<uint32_t>(DeviceShaderResourceGroupData::ResourceType::Count); i++)
        {
            if (CheckBit(sourceUpdateMask, i))
            {
                m_resourceTypeIteration[i] = 0;
            }
        }
    }

    void DeviceShaderResourceGroup::DisableCompilationForAllResourceTypes()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(DeviceShaderResourceGroupData::ResourceType::Count); i++)
        {
            if (CheckBit(m_rhiUpdateMask, i))
            {
                //Ensure that a SRG update is alive for m_updateMaskResetLatency times
                //after SRG compile is called. This is because SRGs are triple buffered
                if (m_resourceTypeIteration[i] >= m_updateMaskResetLatency)
                {
                    m_rhiUpdateMask = RHI::ResetBits(m_rhiUpdateMask, AZ_BIT(i));
                }
                m_resourceTypeIteration[i]++;
            }
        }
    }
    
    bool DeviceShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
    {
        return RHI::CheckBitsAny(m_rhiUpdateMask, resourceTypeMask);
    }

    bool DeviceShaderResourceGroup::IsAnyResourceTypeUpdated() const
    {
        return m_rhiUpdateMask != 0;
    }

    void DeviceShaderResourceGroup::EnableRhiResourceTypeCompilation(const DeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
    {
        m_rhiUpdateMask = AZ::RHI::SetBits(m_rhiUpdateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void DeviceShaderResourceGroup::ResetResourceTypeIteration(const DeviceShaderResourceGroupData::ResourceType resourceType)
    {
        m_resourceTypeIteration[static_cast<uint32_t>(resourceType)] = 0;
    }

    HashValue64 DeviceShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
    {
        return m_viewHash[viewName];
    }

    void DeviceShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
    {
        m_viewHash[viewName] = viewHash;
    }
    
    void DeviceShaderResourceGroup::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        AZ_UNUSED(builder);
    }
}
