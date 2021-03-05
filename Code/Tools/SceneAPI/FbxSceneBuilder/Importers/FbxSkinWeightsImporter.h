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

#include <AzCore/std/containers/vector.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
    }

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

        namespace FbxSceneBuilder
        {
            class FbxSkinWeightsImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxSkinWeightsImporter, "{95FCD291-5E1F-4591-90AD-AB5EA2599C3E}", SceneCore::LoadingComponent);

                FbxSkinWeightsImporter();
                ~FbxSkinWeightsImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportSkinWeights(SceneNodeAppendedContext& context);
                Events::ProcessingResult SetupNamedBoneLinks(FinalizeSceneContext& context);

            protected:
                struct Pending
                {
                    std::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper> m_fbxMesh;
                    AZStd::shared_ptr<const FbxSDKWrapper::FbxSkinWrapper> m_fbxSkin;
                    AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> m_skinWeightData;
                };
                AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> BuildSkinWeightData(
                    const std::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper>& fbxMesh, int skinIndex);

                //! List of skin weights that still need to be filled in. Setting the data for skin weights is
                //! delayed until after the tree has been fully constructed as bones are linked by name, but until
                //! the graph has been fully filled in, those names can change which would break the names recorded
                //! for the skin.
                AZStd::vector<Pending> m_pendingSkinWeights;

                static const AZStd::string s_skinWeightName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ