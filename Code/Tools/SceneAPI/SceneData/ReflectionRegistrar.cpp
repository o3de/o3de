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

#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/OriginRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneData/Rules/SkeletonProxyRule.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>

#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        void RegisterDataTypeReflection(AZ::SerializeContext* context)
        {
            // Check if this library hasn't already been reflected. This can happen as the ResourceCompilerScene needs
            //      to explicitly load and reflect the SceneAPI libraries to discover the available extension, while
            //      Gems with system components need to do the same in the Project Configurator.
            if (!context->IsRemovingReflection() && context->FindClassData(SceneData::MeshGroup::TYPEINFO_Uuid()))
            {
                return;
            }

            // Groups
            SceneData::MeshGroup::Reflect(context);
            SceneData::SkeletonGroup::Reflect(context);
            SceneData::SkinGroup::Reflect(context);
            SceneData::AnimationGroup::Reflect(context);

            // Rules
            SceneData::BlendShapeRule::Reflect(context);
            SceneData::CommentRule::Reflect(context);
            SceneData::LodRule::Reflect(context);
            SceneData::StaticMeshAdvancedRule::Reflect(context);
            SceneData::OriginRule::Reflect(context);
            SceneData::MaterialRule::Reflect(context);
            SceneData::ScriptProcessorRule::Reflect(context);
            SceneData::SkeletonProxyRule::Reflect(context);
            SceneData::SkinMeshAdvancedRule::Reflect(context);
            SceneData::TangentsRule::Reflect(context);

            // Utility
            SceneData::SceneNodeSelectionList::Reflect(context);

            // Graph objects
            context->Class<AZ::SceneData::GraphData::AnimationData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::BlendShapeData>()->Version(1);
            AZ::SceneData::GraphData::BoneData::Reflect(context);
            AZ::SceneData::GraphData::MaterialData::Reflect(context);
            context->Class<AZ::SceneData::GraphData::MeshData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::MeshVertexColorData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::MeshVertexUVData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::MeshVertexTangentData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::MeshVertexBitangentData>()->Version(1);
            AZ::SceneData::GraphData::RootBoneData::Reflect(context);
            context->Class<AZ::SceneData::GraphData::SkinMeshData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::SkinWeightData>()->Version(1);
            AZ::SceneData::GraphData::TransformData::Reflect(context);
        }

        void RegisterDataTypeBehaviorReflection(AZ::BehaviorContext* context)
        {
            AZ::SceneData::GraphData::BoneData::Reflect(context);
            AZ::SceneData::GraphData::MaterialData::Reflect(context);
            AZ::SceneData::GraphData::RootBoneData::Reflect(context);
            AZ::SceneData::GraphData::TransformData::Reflect(context);
        }
    } // namespace SceneAPI
} // namespace AZ
