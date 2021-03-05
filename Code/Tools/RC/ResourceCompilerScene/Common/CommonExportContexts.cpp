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

#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>

namespace AZ
{
    namespace RC
    {
        ContainerExportContext::ContainerExportContext(SceneAPI::Events::ExportEventContext& parent,
            const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, Phase phase)
            : m_scene(parent.GetScene())
            , m_outputDirectory(parent.GetOutputDirectory())
            , m_group(group)
            , m_container(container)
            , m_phase(phase)
        {
        }

        ContainerExportContext::ContainerExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, Phase phase)
            : m_scene(scene)
            , m_outputDirectory(outputDirectory)
            , m_group(group)
            , m_container(container)
            , m_phase(phase)
        {
        }

        ContainerExportContext::ContainerExportContext(const ContainerExportContext& copyContext, Phase phase)
            : m_scene(copyContext.m_scene)
            , m_outputDirectory(copyContext.m_outputDirectory)
            , m_group(copyContext.m_group)
            , m_container(copyContext.m_container)
            , m_phase(phase)
        {
        }

        NodeExportContext::NodeExportContext(ContainerExportContext& parent, CNodeCGF& node, const AZStd::string& nodeName,
            SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex, EPhysicsGeomType physicalizeType, AZStd::string& rootBoneName, Phase phase)
            : ContainerExportContext(parent, phase) 
            , m_node(node)
            , m_nodeName(nodeName)
            , m_nodeIndex(nodeIndex)
            , m_physicalizeType(physicalizeType)
            , m_rootBoneName(rootBoneName)
        {
        }

        NodeExportContext::NodeExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, CNodeCGF& node, 
            const AZStd::string& nodeName, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex, EPhysicsGeomType physicalizeType,
            AZStd::string& rootBoneName, Phase phase)
            : ContainerExportContext(scene, outputDirectory, group, container, phase)
            , m_node(node)
            , m_nodeName(nodeName)
            , m_nodeIndex(nodeIndex)
            , m_physicalizeType(physicalizeType)
            , m_rootBoneName(rootBoneName)
        {
        }

        NodeExportContext::NodeExportContext(const NodeExportContext& copyContext, Phase phase)
            : ContainerExportContext(copyContext, phase)
            , m_node(copyContext.m_node)
            , m_nodeName(copyContext.m_nodeName)
            , m_nodeIndex(copyContext.m_nodeIndex)
            , m_physicalizeType(copyContext.m_physicalizeType)
            , m_rootBoneName(copyContext.m_rootBoneName)
        {
        }

        MeshNodeExportContext::MeshNodeExportContext(NodeExportContext& parent, CMesh& mesh, Phase phase)
            : NodeExportContext(parent, phase)
            , m_mesh(mesh)
        {
        }

        MeshNodeExportContext::MeshNodeExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const SceneAPI::DataTypes::IGroup& group, CContentCGF& container, CNodeCGF& node, 
            const AZStd::string& nodeName, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex, EPhysicsGeomType physicalizeType, 
            AZStd::string& rootBoneName, CMesh& mesh, Phase phase)
            : NodeExportContext(scene, outputDirectory, group, container, node, nodeName, nodeIndex, physicalizeType, rootBoneName, phase)
            , m_mesh(mesh)
        {
        }

        MeshNodeExportContext::MeshNodeExportContext(const MeshNodeExportContext& copyContext, Phase phase)
            : NodeExportContext(copyContext, phase)
            , m_mesh(copyContext.m_mesh)
        {
        }

        TouchBendableMeshNodeExportContext::TouchBendableMeshNodeExportContext(const MeshNodeExportContext& copyContext, AZStd::string& rootBoneName, Phase phase)
            : MeshNodeExportContext(copyContext, phase)
        {
            m_rootBoneName = rootBoneName;
        }

        ResolveRootBoneFromNodeContext::ResolveRootBoneFromNodeContext(
            AZStd::string& result, const SceneAPI::Containers::Scene& scene, SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex)
            : m_scene(scene)
            , m_rootBoneName(result)
            , m_nodeIndex(nodeIndex)
        {
        }

        ResolveRootBoneFromBoneContext::ResolveRootBoneFromBoneContext(
            AZStd::string& result, const SceneAPI::Containers::Scene& scene, const AZStd::string& boneName)
            : m_scene(scene)
            , m_boneName(boneName)
            , m_rootBoneName(result)
        {
        }

        AddBonesToSkinningInfoContext::AddBonesToSkinningInfoContext(
            CSkinningInfo& skinningInfo, const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName)
            : m_scene(scene)
            , m_rootBoneName(rootBoneName)
            , m_skinningInfo(skinningInfo)
        {
        }

        BuildBoneMapContext::BuildBoneMapContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName,
            AZStd::unordered_map<AZStd::string, int>& boneNameIdMap)
            : m_scene(scene)
            , m_rootBoneName(rootBoneName)
            , m_boneNameIdMap(boneNameIdMap)
        {
        }

        SkeletonExportContext::SkeletonExportContext(const SceneAPI::Containers::Scene& scene, const AZStd::string& rootBoneName, CSkinningInfo& skinningInfo,
            [[maybe_unused]] AZStd::unordered_map<AZStd::string, int>& boneNameIdMap, Phase phase)
            : m_scene(scene)
            , m_rootBoneName(rootBoneName)
            , m_skinningInfo(skinningInfo)
            , m_phase(phase)
        {
        }

        SkeletonExportContext::SkeletonExportContext(const SkeletonExportContext& copyContext, Phase phase)
            : m_scene(copyContext.m_scene)
            , m_rootBoneName(copyContext.m_rootBoneName)
            , m_skinningInfo(copyContext.m_skinningInfo)
            , m_phase(phase)
        {
        }
    } // RC
} // AZ
