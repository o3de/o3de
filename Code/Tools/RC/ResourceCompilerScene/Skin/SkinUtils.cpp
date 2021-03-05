

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

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Skin/SkinExportContexts.h>
#include <RC/ResourceCompilerScene/Skin/SkinGroupExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>
#include <RC/ResourceCompilerScene/Skin/SkinUtils.h>
#include <RC/ResourceCompilerScene/Skin/SkinExportContexts.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        void ConfigureSkinContent(CContentCGF& content)
        {
            CExportInfoCGF* exportInfo = content.GetExportInfo();

            exportInfo->bMergeAllNodes = true;
            exportInfo->bUseCustomNormals = false;
            exportInfo->bCompiledCGF = false;
            exportInfo->bHavePhysicsProxy = false;
            exportInfo->bHaveAutoLods = false;
            exportInfo->bNoMesh = true;
            exportInfo->b8WeightsPerVertex = false;
            exportInfo->bWantF32Vertices = false;
            exportInfo->authorToolVersion = 1;
        }

        void MergeToFirstNodeMesh(CContentCGF& content)
        {
            AZ_Assert(content.GetNodeCount() > 0, "Skin Mesh has no node to merge");
            if (content.GetNodeCount() == 0)
            {
                return;
            }
            CMesh* mergedMesh = content.GetNode(0)->pMesh;
            AZ_Assert(mergedMesh, "Failed to retrieve merged mesh for content root node");
            if (!mergedMesh)
            {
                return;
            }
            for (size_t nodeIndex = 0; nodeIndex < content.GetNodeCount(); ++nodeIndex)
            {
                CNodeCGF* node = content.GetNode(nodeIndex);
                AZ_Assert(node, "Failed to retrieve node at index %i", nodeIndex);
                if (!node)
                {
                    continue;
                }
                if (!node->bIdentityMatrix)
                {
                    AZ_Assert(node->pMesh, "No mesh node set on CGF node at index %i", nodeIndex);
                    if (!node->pMesh)
                    {
                        continue;
                    }
                    for (size_t vertexIndex = 0; vertexIndex < node->pMesh->GetVertexCount(); ++vertexIndex)
                    {
                        node->pMesh->m_pPositions[vertexIndex] = node->worldTM.TransformPoint(node->pMesh->m_pPositions[vertexIndex]);
                        node->pMesh->m_pNorms[vertexIndex].RotateSafelyBy(node->worldTM);
                    }
                }
                if (nodeIndex > 0)
                {
                    mergedMesh->Append(*node->pMesh);

                    // Keep color stream in sync size with vertex/normals stream. Reference to CGFNodeMerger::MergeNodes
                    if (mergedMesh->m_streamSize[CMesh::COLORS][0] > 0 && mergedMesh->m_streamSize[CMesh::COLORS][0] < mergedMesh->GetVertexCount())
                    {
                        int colorCount = mergedMesh->m_streamSize[CMesh::COLORS][0];
                        mergedMesh->ReallocStream(CMesh::COLORS, 0, mergedMesh->GetVertexCount());
                        memset(mergedMesh->m_pColor0 + colorCount, 255, (mergedMesh->GetVertexCount() - colorCount) * sizeof(SMeshColor));
                    }
                }
            }

            // We already used the transform during merge, so clear it out
            content.GetNode(0)->worldTM.SetIdentity();
        }

        void RemoveRedundantNodes(CContentCGF& content)
        {
            while (content.GetNodeCount() > 1)
            {
                CNodeCGF* deleteNode = content.GetNode(content.GetNodeCount() - 1);
                content.RemoveNode(deleteNode);
            }
        }

        SceneAPI::Events::ProcessingResult ProcessSkins(SkinGroupExportContext& context, CContentCGF& content, AZStd::vector<AZStd::string>& targetNodes)
        {
            namespace SceneEvents = SceneAPI::Events;
            
            if (targetNodes.empty())
            {
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "No nodes selected for mesh exporting.");
                return SceneEvents::ProcessingResult::Ignored;
            }

            SceneEvents::ProcessingResultCombiner result;

            ContainerExportContext containerContext(context.m_scene, context.m_outputDirectory, context.m_group, content, Phase::Construction);
            result += SceneEvents::Process(containerContext);
            result += SceneEvents::Process<ContainerExportContext>(containerContext, Phase::Filling);

            const EPhysicsGeomType physicalizeType = PHYS_GEOM_TYPE_NONE;
            AZStd::string rootBoneName;
            const SceneAPI::Containers::SceneGraph& graph = context.m_scene.GetGraph();
            
            AZStd::string currentRootBoneName;
            for (const AZStd::string& nodeName : targetNodes)
            {
                AZ_TraceContext("Skin mesh", nodeName);

                SceneAPI::Containers::SceneGraph::NodeIndex index = graph.Find(nodeName.c_str());
                if (index.IsValid())
                {
                    // Pick the target skeleton from the first node, then make sure all the remaining meshes are referencing the same skeleton
                    //      as the skins need to merged to a single mesh at the end.
                    SceneEvents::ProcessingResult rootNameResult = SceneEvents::Process<ResolveRootBoneFromNodeContext>(currentRootBoneName, context.m_scene, index);
                    if (rootNameResult != SceneEvents::ProcessingResult::Success || currentRootBoneName.empty())
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Selected skin has no weight data.");
                        continue;
                    }

                    if (rootBoneName.empty())
                    {
                        rootBoneName = currentRootBoneName;
                        // The skeleton has been established so fill up the skinning information for it as there's still 
                        //      a strong link between skin and skeleton.
                        SceneEvents::ProcessingResult skinInfoResult = SceneEvents::Process<AddBonesToSkinningInfoContext>(
                            *content.GetSkinningInfo(), context.m_scene, rootBoneName);
                        if (skinInfoResult != SceneEvents::ProcessingResult::Success)
                        {
                            // Without the skinning info further processing will cause a crash so early out here.
                            AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to link bones to skin.");
                            return SceneEvents::ProcessingResult::Failure;
                        }
                    }
                    else if (rootBoneName != currentRootBoneName)
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Skin doesn't belong to the same skeleton as the rest of the meshes in the group.");
                        continue;
                    }

                    CNodeCGF* node = new CNodeCGF(); // will be auto deleted by CContentCGF cgf
                    SetNodeName(nodeName, *node);
                    result += SceneAPI::Events::Process<NodeExportContext>(containerContext, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Construction);
                    result += SceneAPI::Events::Process<NodeExportContext>(containerContext, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Filling);
                    content.AddNode(node);
                    result += SceneAPI::Events::Process<NodeExportContext>(containerContext, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Finalizing);
                    
                    currentRootBoneName.clear();
                }
            }

            if (content.GetNodeCount() > 0)
            {
                // CharacterCompiler expects all skin sub-meshes to be merged and stored in a single CNodeCGF
                MergeToFirstNodeMesh(content);
                result += SceneAPI::Events::Process<ContainerExportContext>(containerContext, Phase::Finalizing);
                RemoveRedundantNodes(content);
            }
            else
            {
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "No valid skin information found that could be added to this container.");
                result += SceneAPI::Events::Process<ContainerExportContext>(containerContext, Phase::Finalizing);
            }

            return result.GetResult();
        }
    }
}