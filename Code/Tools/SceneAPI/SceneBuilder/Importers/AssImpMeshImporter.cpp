/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
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
                    serializeContext->Class<AssImpMeshImporter, SceneCore::LoadingComponent>()->Version(3);
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
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
