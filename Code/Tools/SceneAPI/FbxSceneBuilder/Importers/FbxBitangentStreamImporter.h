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
            class MeshVertexBitangentData;
        }
    }

    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
        class FbxVertexBitangentWrapper;
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxBitangentStreamImporter
                : public SceneCore::LoadingComponent
            {
            public:                                
                AZ_COMPONENT(FbxBitangentStreamImporter, "{B68F90E6-9F9D-448F-A874-CABA9F67E5FD}", SceneCore::LoadingComponent);

                FbxBitangentStreamImporter();
                ~FbxBitangentStreamImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportBitangents(SceneNodeAppendedContext& context);

            protected:
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> BuildVertexBitangentData(const FbxSDKWrapper::FbxVertexBitangentWrapper& bitangents,
                    size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
