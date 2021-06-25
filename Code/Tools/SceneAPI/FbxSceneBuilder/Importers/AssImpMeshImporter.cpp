/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            AssImpMeshImporter::AssImpMeshImporter()
            {
                BindToCall(&AssImpMeshImporter::ImportMesh);
            }

            void AssImpMeshImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpMeshImporter, SceneCore::LoadingComponent>()->Version(2);
                }
            }

            Events::ProcessingResult AssImpMeshImporter::ImportMesh(AssImpNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Mesh");

                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if (!context.m_sourceNode.ContainsMesh() || IsSkinnedMesh(*currentNode, *scene))
                {
                    return Events::ProcessingResult::Ignored;
                }

                if (BuildSceneMeshFromAssImpMesh(currentNode, scene, context.m_sourceSceneSystem, context.m_createdData, [] {
                        return AZStd::make_shared<SceneData::GraphData::MeshData>();
                    }))
                {
                    return Events::ProcessingResult::Success;
                }

                return Events::ProcessingResult::Failure;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
