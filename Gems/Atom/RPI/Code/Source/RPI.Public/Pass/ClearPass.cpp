/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/ClearPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

#include <Atom/RPI.Reflect/Pass/ClearPassData.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<ClearPass> ClearPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ClearPass> pass = aznew ClearPass(descriptor);
            return pass;
        }

        ClearPass::ClearPass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
            const ClearPassData* passData = PassUtils::GetPassData<ClearPassData>(descriptor);
            if (passData != nullptr)
            {
                m_clearValue = passData->m_clearValue;
            }
        }

        void ClearPass::InitializeInternal()
        {
            RenderPass::InitializeInternal();

            // Set clear value
            AZ_Assert(GetInputOutputCount() > 0, "ClearPass: Missing InputOutput binding!");
            RPI::PassAttachmentBinding& binding = GetInputOutputBinding(0);
            binding.m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = m_clearValue;
        }
        
    }   // namespace RPI
}   // namespace AZ
