/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ
{
    namespace RPI
    {
        namespace PassUtils
        {
            const PassData* GetPassData(const PassDescriptor& descriptor)
            {
                const PassData* passData = nullptr;

                // Try custom data from PassRequest
                if (descriptor.m_passRequest != nullptr)
                {
                    passData = descriptor.m_passRequest->m_passData.get();
                }
                // Try custom data from PassTemplate
                if (passData == nullptr && descriptor.m_passTemplate != nullptr)
                {
                    passData = descriptor.m_passTemplate->m_passData.get();
                }
                if (passData == nullptr)
                {
                    passData = descriptor.m_passData.get();
                }
                return passData;
            }

            bool BindDataMappingsToSrg(const PassDescriptor& descriptor, ShaderResourceGroup* shaderResourceGroup)
            {
                bool success = true;

                // Apply mappings from PassTemplate
                const RenderPassData* passData = nullptr;
                if (descriptor.m_passTemplate != nullptr)
                {
                    passData = azrtti_cast<const RenderPassData*>(descriptor.m_passTemplate->m_passData.get());
                    if (passData)
                    {
                        success = shaderResourceGroup->ApplyDataMappings(passData->m_mappings);
                    }
                }

                // Apply mappings from PassRequest
                passData = nullptr;
                if (descriptor.m_passRequest != nullptr)
                {
                    passData = azrtti_cast<const RenderPassData*>(descriptor.m_passRequest->m_passData.get());
                    if (passData)
                    {
                        success = success && shaderResourceGroup->ApplyDataMappings(passData->m_mappings);
                    }
                }

                // Apply mappings from custom data in the descriptor
                passData = azrtti_cast<const RenderPassData*>(descriptor.m_passData.get());
                if (passData)
                {
                    success = success && shaderResourceGroup->ApplyDataMappings(passData->m_mappings);
                }

                return success;
            }


        }   // namespace PassUtils
    }   // namespace RPI
}   // namespace AZ
