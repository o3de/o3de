/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Groups/ImportGroup.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/UnmodifiableRule.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneData/Rules/SkeletonProxyRule.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>
#include <SceneAPI/SceneData/Rules/UVsRule.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Rules/TagRule.h>

#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPI/SceneData/GraphData/AnimationData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
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
            //      Gems with system components need to do the same in the Project Manager.
            if (!context->IsRemovingReflection() && context->FindClassData(SceneData::MeshGroup::TYPEINFO_Uuid()))
            {
                return;
            }

            // Groups
            SceneData::ImportGroup::Reflect(context);
            SceneData::MeshGroup::Reflect(context);
            SceneData::SkeletonGroup::Reflect(context);
            SceneData::SkinGroup::Reflect(context);
            SceneData::AnimationGroup::Reflect(context);

            // Rules
            SceneData::BlendShapeRule::Reflect(context);
            SceneData::CommentRule::Reflect(context);
            SceneData::LodRule::Reflect(context);
            SceneData::StaticMeshAdvancedRule::Reflect(context);
            SceneData::MaterialRule::Reflect(context);
            SceneData::UnmodifiableRule::Reflect(context);
            SceneData::ScriptProcessorRule::Reflect(context);
            SceneData::SkeletonProxyRule::Reflect(context);
            SceneData::SkinMeshAdvancedRule::Reflect(context);
            SceneData::TangentsRule::Reflect(context);
            SceneData::UVsRule::Reflect(context);
            SceneData::CoordinateSystemRule::Reflect(context);
            SceneData::TagRule::Reflect(context);

            // Utility
            SceneData::SceneNodeSelectionList::Reflect(context);

            // Graph objects
            AZ::SceneData::GraphData::AnimationData::Reflect(context);
            AZ::SceneData::GraphData::BlendShapeAnimationData::Reflect(context);
            AZ::SceneData::GraphData::BlendShapeData::Reflect(context);
            AZ::SceneData::GraphData::BoneData::Reflect(context);
            AZ::SceneData::GraphData::MaterialData::Reflect(context);
            AZ::SceneData::GraphData::MeshData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexColorData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexUVData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexTangentData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexBitangentData::Reflect(context);
            AZ::SceneData::GraphData::RootBoneData::Reflect(context);
            context->Class<AZ::SceneData::GraphData::SkinMeshData>()->Version(1);
            context->Class<AZ::SceneData::GraphData::SkinWeightData>()->Version(1);
            AZ::SceneData::GraphData::TransformData::Reflect(context);
            AZ::SceneData::GraphData::CustomPropertyData::Reflect(context);
        }

        void RegisterDataTypeBehaviorReflection(AZ::BehaviorContext* context)
        {
            AZ::SceneData::GraphData::BoneData::Reflect(context);
            AZ::SceneData::GraphData::MaterialData::Reflect(context);
            AZ::SceneData::GraphData::RootBoneData::Reflect(context);
            AZ::SceneData::GraphData::TransformData::Reflect(context);
            AZ::SceneData::GraphData::MeshData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexColorData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexUVData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexTangentData::Reflect(context);
            AZ::SceneData::GraphData::MeshVertexBitangentData::Reflect(context);
            AZ::SceneData::GraphData::AnimationData::Reflect(context);
            AZ::SceneData::GraphData::BlendShapeAnimationData::Reflect(context);
            AZ::SceneData::GraphData::BlendShapeData::Reflect(context);
            AZ::SceneData::GraphData::CustomPropertyData::Reflect(context);
        }
    } // namespace SceneAPI
} // namespace AZ
