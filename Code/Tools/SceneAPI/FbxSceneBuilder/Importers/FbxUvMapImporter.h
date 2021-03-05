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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshVertexUVData;
        }
    }

    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
        class FbxUVWrapper;
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxUvMapImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxUvMapImporter, "{B16CD69D-3C0C-4FE2-B481-1084B1C36242}", SceneCore::LoadingComponent);

                FbxUvMapImporter();
                ~FbxUvMapImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportUvMaps(SceneNodeAppendedContext& context);

            protected:
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> BuildVertexUVData(const FbxSDKWrapper::FbxUVWrapper& uvs,
                    size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ