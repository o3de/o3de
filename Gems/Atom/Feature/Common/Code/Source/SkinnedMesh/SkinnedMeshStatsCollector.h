/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            void AddVerticesToSceneStats(size_t vertexCount);

            SkinnedMeshSceneStats m_sceneStats;
            // Use sets to ensure we're not double-counting any resources that are shared
            AZStd::unordered_set<RPI::Buffer*> m_sceneBoneTransforms;
            SkinnedMeshFeatureProcessor* m_featureProcessor;
        };
    } // namespace Render
} // namespace AZ
