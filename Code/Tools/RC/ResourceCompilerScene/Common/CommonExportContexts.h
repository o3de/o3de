#pragma once

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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>
#include <CryHeaders.h>

class CContentCGF;
struct CNodeCGF;
class CMesh;
struct CSkinningInfo;

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGroup;
            class ISceneNodeSelectionList;
        }
    }
    namespace RC
    {
        // Called while creating, filling and finalizing a CContentCGF container.
        struct ContainerExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ContainerExportContext, "{667A9E60-F3AA-45E1-8E66-05B0C971A094}", SceneAPI::Events::ICallContext);
            ContainerExportContext(SceneAPI::Events::ExportEventContext& parent,
                const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, Phase phase);
            ContainerExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
                const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, Phase phase);
            ContainerExportContext(const ContainerExportContext& copyContext, Phase phase);
            ContainerExportContext(const ContainerExportContext& copyContext) = delete;
            ~ContainerExportContext() override = default;

            ContainerExportContext& operator=(const ContainerExportContext& other) = delete;

            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_outputDirectory;
            const SceneAPI::DataTypes::IGroup& m_group;
            CContentCGF& m_container;
            const Phase m_phase;
        };

        // Called when a new CNode is added to a CContentCGF container.
        struct NodeExportContext
            : public ContainerExportContext
        {
            AZ_RTTI(NodeExportContext, "{A7D130C6-2CB2-47AC-9D9C-969FA473DFDA}", ContainerExportContext);

            NodeExportContext(ContainerExportContext& parent, CNodeCGF& node, const AZStd::string& nodeName,
                SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex, EPhysicsGeomType physicalizeType, AZStd::string& rootBoneName, Phase phase);
            NodeExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
                const SceneAPI::DataTypes::IGroup& group,
                CContentCGF& container, CNodeCGF& node, const AZStd::string& nodeName, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex,
                EPhysicsGeomType physicalizeType, AZStd::string& rootBoneName, Phase phase);
            NodeExportContext(const NodeExportContext& copyContext, Phase phase);
            NodeExportContext(const NodeExportContext& copyContext) = delete;
            ~NodeExportContext() override = default;

            NodeExportContext& operator=(const NodeExportContext& other) = delete;

            CNodeCGF& m_node;
            const AZStd::string& m_nodeName;
            SceneAPI::Containers::SceneGraph::NodeIndex m_nodeIndex;
            EPhysicsGeomType m_physicalizeType;
            AZStd::string& m_rootBoneName;
        };

        // Called when new mesh data was added to a CNode in a CContentCGF container.
        struct MeshNodeExportContext
            : public NodeExportContext
        {
            AZ_RTTI(MeshNodeExportContext, "{D39D08D6-8EB5-4058-B9D7-BED4EB460555}", NodeExportContext);

            MeshNodeExportContext(NodeExportContext& parent, CMesh& mesh, Phase phase);
            MeshNodeExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
                const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, CNodeCGF& node,
                const AZStd::string& nodeName, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex, EPhysicsGeomType physicalizeType,
                AZStd::string& rootBoneName, CMesh& mesh, Phase phase);
            MeshNodeExportContext(const MeshNodeExportContext& copyContext, Phase phase);
            MeshNodeExportContext(const MeshNodeExportContext& copyContext) = delete;
            ~MeshNodeExportContext() override = default;
            
            MeshNodeExportContext& operator=(const MeshNodeExportContext& other) = delete;

            CMesh& m_mesh;
        };

        struct TouchBendableMeshNodeExportContext
            : public MeshNodeExportContext
        {
            AZ_RTTI(TouchBendableMeshNodeExportContext, "{A3370E01-EF04-4F5A-95F3-5B9ADFEFD2F0}", MeshNodeExportContext);

            TouchBendableMeshNodeExportContext(const MeshNodeExportContext& copyContext, AZStd::string& rootBoneName, Phase phase);
            TouchBendableMeshNodeExportContext(const TouchBendableMeshNodeExportContext& copyContext) = delete;
            ~TouchBendableMeshNodeExportContext() override = default;

            TouchBendableMeshNodeExportContext& operator=(const TouchBendableMeshNodeExportContext& other) = delete;
        };

        // Finds a root bone of the skeleton that is referenced by the given node.
        struct ResolveRootBoneFromNodeContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ResolveRootBoneFromNodeContext, "{7BA28E30-E313-4B55-8200-C3BDD4EEE240}", SceneAPI::Events::ICallContext);

            ResolveRootBoneFromNodeContext(AZStd::string& result, const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex);
            ~ResolveRootBoneFromNodeContext() override = default;

            const SceneAPI::Containers::Scene& m_scene;
            AZStd::string& m_rootBoneName;
            SceneAPI::Containers::SceneGraph::NodeIndex m_nodeIndex;
        };

        // Finds a root bone of the skeleton that contains m_boneName. If the given bone name is not a fully specified
        //      path the graph will be searched for the node that's closest to the root that matches the name.
        struct ResolveRootBoneFromBoneContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ResolveRootBoneFromBoneContext, "{DCA7DE80-28D8-42B1-845D-2FD596E7B8D5}", SceneAPI::Events::ICallContext);

            ResolveRootBoneFromBoneContext(AZStd::string& result, const SceneAPI::Containers::Scene& scene, const AZStd::string& boneName);
            ~ResolveRootBoneFromBoneContext() override = default;

            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_boneName;
            AZStd::string& m_rootBoneName;
        };

        struct AddBonesToSkinningInfoContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(AddBonesToSkinningInfoContext, "{18BFBCA3-DE2D-45BF-A776-E93A991C467E}", SceneAPI::Events::ICallContext);

            AddBonesToSkinningInfoContext(CSkinningInfo& skinningInfo, const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName);
            ~AddBonesToSkinningInfoContext() override = default;

            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_rootBoneName;
            CSkinningInfo& m_skinningInfo;
        };

        struct BuildBoneMapContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(BuildBoneMapContext, "{9D9EE333-EC8C-4811-AB82-CC3B414E334C}", SceneAPI::Events::ICallContext);

            BuildBoneMapContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName, 
                AZStd::unordered_map<AZStd::string, int>& boneNameIdMap);
            ~BuildBoneMapContext() override = default;

            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_rootBoneName;
            AZStd::unordered_map<AZStd::string, int>& m_boneNameIdMap;
        };

        struct SkeletonExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(SkeletonExportContext, "{40512752-150F-4BAF-BC4E-01016DAE5088}", SceneAPI::Events::ICallContext);

            SkeletonExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName, CSkinningInfo& skinningInfo,
                AZStd::unordered_map<AZStd::string, int>& boneNameIdMap, Phase phase);
            SkeletonExportContext(const SkeletonExportContext& copyContext, Phase phase);
            SkeletonExportContext(const SkeletonExportContext& copyContext) = delete;
            ~SkeletonExportContext() override = default;

            SkeletonExportContext& operator=(const SkeletonExportContext& other) = delete;

            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_rootBoneName;
            CSkinningInfo& m_skinningInfo;
            const Phase m_phase;
        };
    } // RC
} // AZ
