/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/sort.h>

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

#include <Atom/RPI.Public/Pass/Pass.h>
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

            AZStd::shared_ptr<PassData> GetPassDataPtr(const PassDescriptor& descriptor)
            {
                AZStd::shared_ptr<PassData> passData = nullptr;

                if (descriptor.m_passRequest != nullptr)
                {
                    passData = descriptor.m_passRequest->m_passData;
                }
                if (passData == nullptr && descriptor.m_passTemplate != nullptr)
                {
                    passData = descriptor.m_passTemplate->m_passData;
                }
                if (passData == nullptr)
                {
                    passData = descriptor.m_passData;
                }
                return passData;
            }

            void ExtractPipelineGlobalConnections(const AZStd::shared_ptr<PassData>& passData, PipelineGlobalConnectionList& outList)
            {
                for (const PipelineGlobalConnection& connection : passData->m_pipelineGlobalConnections)
                {
                    outList.push_back(connection);
                }
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

            // Sort so passes with less depth (closer to the root) are first. Used when changes 
            // in the parent passes can affect the child passes, like with attachment building.
            void SortPassListAscending(AZStd::vector< Ptr<Pass> >& passList)
            {
                AZStd::sort(passList.begin(), passList.end(),
                    [](const Ptr<Pass>& lhs, const Ptr<Pass>& rhs)
                    {
                        if ((lhs->GetTreeDepth() == rhs->GetTreeDepth()))
                        {
                            return lhs->GetParentChildIndex() < rhs->GetParentChildIndex();
                        }

                        return (lhs->GetTreeDepth() < rhs->GetTreeDepth());
                    });
            }

            // Sort so passes with greater depth (further from the root) get called first. Used in the case of
            // delete, as we want to avoid deleting the parent first since this invalidates the child pointer.
            void SortPassListDescending(AZStd::vector< Ptr<Pass> >& passList)
            {
                AZStd::sort(passList.begin(), passList.end(),
                    [](const Ptr<Pass>& lhs, const Ptr<Pass>& rhs)
                    {
                        if ((lhs->GetTreeDepth() == rhs->GetTreeDepth()))
                        {
                            return lhs->GetParentChildIndex() > rhs->GetParentChildIndex();
                        }
                        return (lhs->GetTreeDepth() > rhs->GetTreeDepth());
                    }
                );
            }
        }
    }
}
