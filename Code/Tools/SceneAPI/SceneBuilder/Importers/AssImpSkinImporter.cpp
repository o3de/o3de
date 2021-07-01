/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpSkinImporter.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            AssImpSkinImporter::AssImpSkinImporter()
            {
                BindToCall(&AssImpSkinImporter::ImportSkin);
            }

            void AssImpSkinImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpSkinImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult AssImpSkinImporter::ImportSkin(AssImpNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Skin");

                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                if (!context.m_sourceNode.ContainsMesh() || !IsSkinnedMesh(*currentNode, *scene))
                {
                    return Events::ProcessingResult::Ignored;
                }

                if (BuildSceneMeshFromAssImpMesh(currentNode, scene, context.m_sourceSceneSystem, context.m_createdData, [] {
                        return AZStd::make_shared<SceneData::GraphData::SkinMeshData>();
                    }))
                {
                    return Events::ProcessingResult::Success;
                }

                return Events::ProcessingResult::Failure;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
