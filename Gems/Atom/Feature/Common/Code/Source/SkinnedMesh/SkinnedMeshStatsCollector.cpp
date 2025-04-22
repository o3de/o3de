/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshStatsCollector.h>

#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshStatsCollector::SkinnedMeshStatsCollector(SkinnedMeshFeatureProcessor* featureProcessor)
            : m_featureProcessor(featureProcessor)
        {
            AZ_Assert(m_featureProcessor, "Attempting to create a SkinnedMeshStatsCollector without an associated SkinnedMeshFeatureProcessor.");
            AZ_Assert(m_featureProcessor->GetParentScene(), "SkinnedMeshFeatureProcessor does not have a parent scene");

            SkinnedMeshStatsRequestBus::Handler::BusConnect(m_featureProcessor->GetParentScene()->GetId());
        }

        SkinnedMeshSceneStats SkinnedMeshStatsCollector::GetSceneStats()
        {
            m_sceneStats.skinnedMeshRenderProxyCount = m_featureProcessor->m_renderProxies.size();

            for (const SkinnedMeshRenderProxy& renderProxy : m_featureProcessor->m_renderProxies)
            {
                for (uint32_t lodIndex = 0; lodIndex < renderProxy.GetLodCount(); ++lodIndex)
                {
                    for (const AZStd::unique_ptr<SkinnedMeshDispatchItem>& dispatchItem : renderProxy.GetDispatchItems(lodIndex))
                    {
                        AddDispatchItemToSceneStats(dispatchItem);
                    }
                }
            }

            // Clear all the stats collection before returning the results so we are not
            // unnecessarily holding onto any references to resources
            SkinnedMeshSceneStats results = m_sceneStats;
            ResetAllStats();

            return results;
        }

        void SkinnedMeshStatsCollector::ResetAllStats()
        {
            m_sceneStats = SkinnedMeshSceneStats();
            m_sceneBoneTransforms.clear();
        }

        void SkinnedMeshStatsCollector::AddDispatchItemToSceneStats(const AZStd::unique_ptr<SkinnedMeshDispatchItem>& dispatchItem)
        {
            m_sceneStats.dispatchItemCount++;
            AddBonesToSceneStats(dispatchItem->GetBoneTransforms());
            AddVerticesToSceneStats(aznumeric_cast<size_t>(dispatchItem->GetVertexCount()));
        }

        void SkinnedMeshStatsCollector::AddBonesToSceneStats(const Data::Instance<RPI::Buffer>& boneTransformBuffer)
        {
            // Oftentimes, different lods and sub-meshes of the same model share the same bone transforms. Use a set to ensure we're not duplicating bone count
            const auto& iter = m_sceneBoneTransforms.find(boneTransformBuffer.get());
            if (iter == m_sceneBoneTransforms.end())
            {
                m_sceneBoneTransforms.insert(boneTransformBuffer.get());
                if (boneTransformBuffer && boneTransformBuffer->GetBufferView())
                {
                    m_sceneStats.boneCount += aznumeric_cast<size_t>(boneTransformBuffer->GetBufferView()->GetDescriptor().m_elementCount);
                }
            }
        }

        void SkinnedMeshStatsCollector::AddVerticesToSceneStats(size_t vertexCount)
        {
            // This counts the number of skinned vertices
            m_sceneStats.vertexCount += vertexCount;
        }

    } // namespace Render
} // namespace AZ
