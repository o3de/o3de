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

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

#include <RCExt/CoordinateSystemConverter.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshData;
            class IBlendShapeRule;
            class IMeshVertexColorData;
        }
    }
    namespace GFxFramework
    {
        class IMaterialGroup;
    }

    class Transform;
}

namespace EMotionFX
{
    class Actor;
    class Node;
    class Mesh;
    class MeshBuilder;
    class MeshBuilderSkinningInfo;
    class MeshBuilderVertexAttributeLayerUInt32;

    namespace Pipeline
    {
        struct ActorBuilderContext;

        namespace Group
        {
            class IActorGroup;
        }

        struct ActorSettings
        {
            bool m_loadMeshes;                      /* Set to false if you wish to disable loading any meshes. */
            bool m_loadStandardMaterialLayers;      /* Set to false if you wish to disable loading any standard material layers. */
            bool m_loadSkinningInfo;
            AZ::u32 m_maxWeightsPerVertex;
            float m_weightThreshold;                /* removes skinning influences below this threshold and re-normalize others. */

            ActorSettings()
                : m_loadMeshes(true)
                , m_loadStandardMaterialLayers(true)
                , m_loadSkinningInfo(true)
                , m_maxWeightsPerVertex(4)
                , m_weightThreshold(0.001f)
            {
            }
        };

        class ActorBuilder
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorBuilder, "{76E1AB76-0861-457D-B100-AFBA154B17FA}", AZ::SceneAPI::SceneCore::ExportingComponent);

            using BoneNameEmfxIndexMap = AZStd::unordered_map<AZStd::string, AZ::u32>;

            ActorBuilder();
            ~ActorBuilder() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult BuildActor(ActorBuilderContext& context);

        protected:
            virtual void InstantiateMaterialGroup();

        private:
            struct NodeIndexHasher
            {
                AZStd::size_t operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
                {
                    return nodeIndex.AsNumber();
                }
            };
            struct NodeIndexComparator
            {
                bool operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& a,
                    const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& b) const
                {
                    return a == b;
                }
            };
            using NodeIndexSet = AZStd::unordered_set<AZ::SceneAPI::Containers::SceneGraph::NodeIndex, NodeIndexHasher, NodeIndexComparator>;

        private:
            void BuildPreExportStructure(ActorBuilderContext& context, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& rootBoneNodeIndex, const NodeIndexSet& selectedMeshNodeIndices,
                AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>& outNodeIndices, BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap);

            void BuildMesh(const ActorBuilderContext& context, EMotionFX::Node* emfxNode, AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData> meshData,
                const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings, 
                const CoordinateSystemConverter& coordSysConverter, AZ::u8 lodLevel);

            EMotionFX::MeshBuilderSkinningInfo* ExtractSkinningInfo(AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData> meshData,
                const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings);

            void CreateSkinningMeshDeformer(EMotionFX::Actor* actor, EMotionFX::Node* node, EMotionFX::Mesh* mesh, EMotionFX::MeshBuilderSkinningInfo* skinningInfo);

            void ExtractActorSettings(const Group::IActorGroup& actorGroup, ActorSettings& outSettings);

            bool GetMaterialInfoForActorGroup(ActorBuilderContext& context);
            void SetupMaterialDataForMesh(const ActorBuilderContext& context, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex);

            AZ::SceneAPI::DataTypes::IMeshVertexColorData* FindVertexColorData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZStd::string& colorNodeName);

            void GetNodeIndicesOfSelectedBaseMeshes(ActorBuilderContext& context, NodeIndexSet& meshNodeIndexSet) const;
            bool GetIsMorphed(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::SceneAPI::DataTypes::IBlendShapeRule* morphTargetRule) const;

            AZStd::string_view RemoveLODSuffix(const AZStd::string_view& lodName);

        protected:
            AZStd::shared_ptr<AZ::GFxFramework::IMaterialGroup> m_materialGroup;
        private:
            AZStd::vector<AZ::u32> m_materialIndexMapForMesh;
        };
    } // namespace Pipeline
} // namespace EMotionFX
