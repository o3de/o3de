/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/utils.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/DescriptorSet.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<ShaderResourceGroup> ShaderResourceGroup::Create()
        {
            return aznew ShaderResourceGroup();
        }

        void ShaderResourceGroup::UpdateCompiledDataIndex(uint64_t frameIteration)
        {
            // Check that this is a new frame compilation.
            if (frameIteration != m_lastCompileFrameIteration)
            {
                m_compiledDataIndex = (m_compiledDataIndex + 1) % m_compiledData.size();
            }
            m_lastCompileFrameIteration = frameIteration;
        }

        const DescriptorSet& ShaderResourceGroup::GetCompiledData() const
        {
            return *m_compiledData[m_compiledDataIndex];
        }

        uint32_t ShaderResourceGroup::GetCompileDataIndex() const
        {
            return m_compiledDataIndex;
        }

        uint64_t ShaderResourceGroup::GetLastCompileFrameIteration() const
        {
            return m_lastCompileFrameIteration;
        }
    }
}
