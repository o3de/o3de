/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>

namespace AZ::RHI
{
    void ShaderResourceGroup::Compile(const ShaderResourceGroupData& groupData, CompileMode compileMode /*= CompileMode::Async*/)
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

    uint32_t ShaderResourceGroup::GetBindingSlot() const
    {
        return m_bindingSlot;
    }

    bool ShaderResourceGroup::IsQueuedForCompile() const
    {
        return m_isQueuedForCompile;
    }

    const ShaderResourceGroupPool* ShaderResourceGroup::GetPool() const
    {
        return static_cast<const ShaderResourceGroupPool*>(Resource::GetPool());
    }

    ShaderResourceGroupPool* ShaderResourceGroup::GetPool()
    {
        return static_cast<ShaderResourceGroupPool*>(Resource::GetPool());
    }

    const ShaderResourceGroupData& ShaderResourceGroup::GetData() const
    {
        return m_data;
    }

    void ShaderResourceGroup::SetData(const ShaderResourceGroupData& data)
    {
        m_data = data;
        uint32_t sourceUpdateMask = data.GetUpdateMask();
            
        //RHI has it's own copy of update mask that is reset after Compile is called m_updateMaskResetLatency times.
        m_rhiUpdateMask |= sourceUpdateMask;
        for (uint32_t i = 0; i < static_cast<uint32_t>(ShaderResourceGroupData::ResourceType::Count); i++)
        {
            if (RHI::CheckBit(sourceUpdateMask, static_cast<AZ::u8>(i)))
            {
                m_resourceTypeIteration[i] = 0;
            }
        }
    }

    void ShaderResourceGroup::DisableCompilationForAllResourceTypes()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(ShaderResourceGroupData::ResourceType::Count); i++)
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
    
    bool ShaderResourceGroup::IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const
    {
        return RHI::CheckBitsAny(m_rhiUpdateMask, resourceTypeMask);
    }

    bool ShaderResourceGroup::IsAnyResourceTypeUpdated() const
    {
        return m_rhiUpdateMask != 0;
    }

    void ShaderResourceGroup::EnableRhiResourceTypeCompilation(const ShaderResourceGroupData::ResourceTypeMask resourceTypeMask)
    {
        m_rhiUpdateMask = AZ::RHI::SetBits(m_rhiUpdateMask, static_cast<uint32_t>(resourceTypeMask));
    }

    void ShaderResourceGroup::ResetResourceTypeIteration(const ShaderResourceGroupData::ResourceType resourceType)
    {
        m_resourceTypeIteration[static_cast<uint32_t>(resourceType)] = 0;
    }

    HashValue64 ShaderResourceGroup::GetViewHash(const AZ::Name& viewName)
    {
        return m_viewHash[viewName];
    }

    void ShaderResourceGroup::UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash)
    {
        m_viewHash[viewName] = viewHash;
    }
    
    void ShaderResourceGroup::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        AZ_UNUSED(builder);
    }
}
