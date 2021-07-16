/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        struct ActorBuilderContext;

        class ActorBuilder
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorBuilder, "{76E1AB76-0861-457D-B100-AFBA154B17FA}", AZ::SceneAPI::SceneCore::ExportingComponent);

            using BoneNameEmfxIndexMap = AZStd::unordered_map<AZStd::string, AZ::u32>;

            ActorBuilder();
            ~ActorBuilder() override = default;

            static void Reflect(AZ::ReflectContext* context);
            AZ::SceneAPI::Events::ProcessingResult BuildActor(ActorBuilderContext& context);

        private:
            void BuildPreExportStructure(ActorBuilderContext& context,
                const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& rootBoneNodeIndex,
                AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>& outNodeIndices,
                AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>& outMeshIndices,
                BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap);
        };
    } // namespace Pipeline
} // namespace EMotionFX
