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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MaterialData;
        }
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxMaterialImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxMaterialImporter, "{E1DF4182-793D-4188-B833-1236D33CCEB4}", SceneCore::LoadingComponent);

                FbxMaterialImporter();
                ~FbxMaterialImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportMaterials(SceneNodeAppendedContext& context);

            protected:
                AZStd::shared_ptr<SceneData::GraphData::MaterialData> BuildMaterial(FbxSDKWrapper::FbxNodeWrapper& node, int materialIndex) const;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
