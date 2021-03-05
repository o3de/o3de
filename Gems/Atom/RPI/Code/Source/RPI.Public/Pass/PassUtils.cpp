/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

                // Apply mappings from PassRequest
                const RenderPassData* passData = nullptr;
                if (descriptor.m_passRequest != nullptr)
                {
                    passData = azrtti_cast<const RenderPassData*>(descriptor.m_passRequest->m_passData.get());
                    if (passData)
                    {
                        success = shaderResourceGroup->ApplyDataMappings(passData->m_mappings);
                    }
                }

                // Apply mappings from PassTemplate
                passData = nullptr;
                if (descriptor.m_passTemplate != nullptr)
                {
                    passData = azrtti_cast<const RenderPassData*>(descriptor.m_passTemplate->m_passData.get());
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