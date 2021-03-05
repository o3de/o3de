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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxMeshImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/FbxMeshImporterUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxMeshImporter::FbxMeshImporter()
            {
                BindToCall(&FbxMeshImporter::ImportMesh);
            }

            void FbxMeshImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxMeshImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxMeshImporter::ImportMesh(FbxNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Mesh");
                if (!context.m_sourceNode.GetMesh() ||
                    IsSkinnedMesh(context.m_sourceNode))
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::shared_ptr<SceneData::GraphData::MeshData> createdData = 
                    AZStd::make_shared<SceneData::GraphData::MeshData>();

                if (BuildSceneMeshFromFbxMesh(createdData, *context.m_sourceNode.GetMesh(), context.m_sourceSceneSystem))
                {
                    context.m_createdData.push_back(std::move(createdData));
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Failure;
                }
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ