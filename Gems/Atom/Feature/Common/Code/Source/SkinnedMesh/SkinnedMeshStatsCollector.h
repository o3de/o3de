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

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshStatsBus.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshFeatureProcessor;

        //! Implements the SkinnedMeshStatsRequestBus for collecting stats about skinned mesh usage in a scene
        class SkinnedMeshStatsCollector
            : public SkinnedMeshStatsRequestBus::Handler
        {
        public:
            SkinnedMeshStatsCollector() = delete;
            SkinnedMeshStatsCollector(SkinnedMeshFeatureProcessor* featureProcessor);

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // SkinnedMeshStatsRequestBus::Handler overrides...
            //! GetSceneStats re-calculates all the scene stats on demand. Requesting them every frame means re-calculating them every frame
            virtual SkinnedMeshSceneStats GetSceneStats() override;

        private:
            void ResetAllStats();
            void AddDispatchItemToSceneStats(const AZStd::unique_ptr<SkinnedMeshDispatchItem>& dispatchItem);
            void AddBonesToSceneStats(const Data::Instance<RPI::Buffer>& boneTransformBuffer);
            void AddReadOnlyBufferViewsToSceneStats(const AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>>& sourceUnskinnedBufferViews);
            void AddWritableBufferViewsToSceneStats(const AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>>& targetSkinnedBufferViews);
            void AddVerticesToSceneStats(size_t vertexCount);

            SkinnedMeshSceneStats m_sceneStats;
            // Use sets to ensure we're not double-counting any resources that are shared
            AZStd::unordered_set<RPI::Buffer*> m_sceneBoneTransforms;
            AZStd::unordered_set<RHI::BufferView*> m_sceneReadOnlyBufferViews;
            AZStd::unordered_set<RHI::BufferView*> m_sceneWritableBufferViews;
            SkinnedMeshFeatureProcessor* m_featureProcessor;
        };
    } // namespace Render
} // namespace AZ
