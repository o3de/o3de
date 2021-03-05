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
            class MeshVertexTangentData;
        }
    }

    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
        class FbxVertexTangentWrapper;
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxTangentStreamImporter
                : public SceneCore::LoadingComponent
            {
            public:                
                AZ_COMPONENT(FbxTangentStreamImporter, "{70F3A9F5-5BB1-4FE2-BD63-A60C2DCA4589}", SceneCore::LoadingComponent);

                FbxTangentStreamImporter();
                ~FbxTangentStreamImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportTangents(SceneNodeAppendedContext& context);

            protected:
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> BuildVertexTangentData(const FbxSDKWrapper::FbxVertexTangentWrapper& tangents, 
                    size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ