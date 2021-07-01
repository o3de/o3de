/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>

namespace AZ
{
    namespace RHI
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
        }

        void ShaderResourceGroup::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
        {
            AZ_UNUSED(builder);
        }
    }
}
