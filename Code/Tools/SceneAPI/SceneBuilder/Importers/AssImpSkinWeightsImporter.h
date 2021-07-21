/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <assimp/scene.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SkinWeightData;
        }
    }

    namespace SceneAPI
    {
        namespace Events
        {
            class PostImportEventContext;
        }

        namespace SceneBuilder
        {
            class AssImpSkinWeightsImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(AssImpSkinWeightsImporter, "{79B5E863-C155-473A-BC0D-B85F8D8303EB}", SceneCore::LoadingComponent);

                AssImpSkinWeightsImporter();
                ~AssImpSkinWeightsImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportSkinWeights(AssImpSceneNodeAppendedContext& context);
                Events::ProcessingResult SetupNamedBoneLinks(AssImpFinalizeSceneContext& context);

            protected:
                struct Pending
                {
                    const aiBone* m_bone = nullptr;
                    unsigned m_numVertices = 0;
                    unsigned m_vertOffset = 0;
                    AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> m_skinWeightData;
                };

                //! List of skin weights that still need to be filled in. Setting the data for skin weights is
                //! delayed until after the tree has been fully constructed as bones are linked by name, but until
                //! the graph has been fully filled in, those names can change which would break the names recorded
                //! for the skin.
                AZStd::vector<Pending> m_pendingSkinWeights;

                static const AZStd::string s_skinWeightName;
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
