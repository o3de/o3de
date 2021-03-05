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
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBoneImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxTransformImporter.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxBoneImporter::FbxBoneImporter()
            {
                BindToCall(&FbxBoneImporter::ImportBone);
            }

            void FbxBoneImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxBoneImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxBoneImporter::ImportBone(FbxNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Bone");

                if (!context.m_sourceNode.IsBone())
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::shared_ptr<DataTypes::IGraphObject> boneGraphData;

                // If the current scene node (our eventual parent) contains bone data, we are not a root bone
                AZStd::shared_ptr<SceneData::GraphData::BoneData> createdBoneData;

                if (NodeHasAncestorOfType(context.m_scene.GetGraph(), context.m_currentGraphPosition,
                    DataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    createdBoneData = AZStd::make_shared<SceneData::GraphData::BoneData>();
                }
                else
                {
                    createdBoneData = AZStd::make_shared<SceneData::GraphData::RootBoneData>();
                }

                SceneAPI::DataTypes::MatrixType globalTransform = context.m_sourceNode.EvaluateGlobalTransform();

                context.m_sourceSceneSystem.SwapTransformForUpAxis(globalTransform);

                context.m_sourceSceneSystem.ConvertBoneUnit(globalTransform);

                createdBoneData->SetWorldTransform(globalTransform);

                context.m_createdData.push_back(AZStd::move(createdBoneData));

                return Events::ProcessingResult::Success;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
