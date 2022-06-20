/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/SlowClearPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

#include <Atom/RPI.Reflect/Pass/SlowClearPassData.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<SlowClearPass> SlowClearPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<SlowClearPass> pass = aznew SlowClearPass(descriptor);
            return pass;
        }

        SlowClearPass::SlowClearPass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
            const SlowClearPassData* passData = PassUtils::GetPassData<SlowClearPassData>(descriptor);
            if (passData != nullptr)
            {
                m_clearValue = passData->m_clearValue;
            }
        }

        void SlowClearPass::InitializeInternal()
        {
            RenderPass::InitializeInternal();

            // Set clear value
            AZ_Assert(GetInputOutputCount() > 0, "SlowClearPass: Missing InputOutput binding!");
            RPI::PassAttachmentBinding& binding = GetInputOutputBinding(0);
            binding.m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = m_clearValue;
        }
        
    }   // namespace RPI
}   // namespace AZ
