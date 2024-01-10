/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>

namespace AZ::RHI
{
    void SingleDeviceShaderResourceGroup::Compile(const SingleDeviceShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
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

    uint32_t SingleDeviceShaderResourceGroup::GetBindingSlot() const
    {
        return m_bindingSlot;
    }

    bool SingleDeviceShaderResourceGroup::IsQueuedForCompile() const
    {
        return m_isQueuedForCompile;
    }

    const SingleDeviceShaderResourceGroupPool* SingleDeviceShaderResourceGroup::GetPool() const
    {
        return static_cast<const SingleDeviceShaderResourceGroupPool*>(SingleDeviceResource::GetPool());
    }

    SingleDeviceShaderResourceGroupPool* SingleDeviceShaderResourceGroup::GetPool()
    {
        return static_cast<SingleDeviceShaderResourceGroupPool*>(SingleDeviceResource::GetPool());
    }

    const SingleDeviceShaderResourceGroupData& SingleDeviceShaderResourceGroup::GetData() const
    {
        return m_data;
    }

    void SingleDeviceShaderResourceGroup::SetData(const SingleDeviceShaderResourceGroupData& data)
    {
        m_data = data;
        uint32_t sourceUpdateMask = data.GetUpdateMask();
            
        //RHI has it's own copy of update mask that is reset after Compile is called m_updateMaskResetLatency times.
        m_rhiUpdateMask |= sourceUpdateMask;
        for (uint32_t i = 0; i < static_cast<uint32_t>(SingleDeviceShaderResourceGroupData::ResourceType::Count); i++)
        {
            if (RHI::CheckBit(sourceUpdateMask, static_cast<AZ::u8>(i)))
            {
                m_resourceTypeIteration[i] = 0;
            }
        }
    }

    void SingleDeviceShaderResourceGroup::DisableCompilationForAllResourceTypes()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(SingleDeviceShaderResourceGroupData::ResourceType::Count); i++)
        {
            if (RHI::CheckBit(m_rhiUpdateMask, static_cast<AZ::u8>(i)))
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
    
    bool SingleDeviceShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
    {
        return RHI::CheckBitsAny(m_rhiUpdateMask, resourceTypeMask);
    }

    bool SingleDeviceShaderResourceGroup::IsAnyResourceTypeUpdated() const
    {
        return m_rhiUpdateMask != 0;
    }

    void SingleDeviceShaderResourceGroup::EnableRhiResourceTypeCompilation(const SingleDeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
    {
        m_rhiUpdateMask = AZ::RHI::SetBits(m_rhiUpdateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void SingleDeviceShaderResourceGroup::ResetResourceTypeIteration(const SingleDeviceShaderResourceGroupData::ResourceType resourceType)
    {
        m_resourceTypeIteration[static_cast<uint32_t>(resourceType)] = 0;
    }

    HashValue64 SingleDeviceShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
    {
        return m_viewHash[viewName];
    }

    void SingleDeviceShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
    {
        m_viewHash[viewName] = viewHash;
    }
    
    void SingleDeviceShaderResourceGroup::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        AZ_UNUSED(builder);
    }
}
