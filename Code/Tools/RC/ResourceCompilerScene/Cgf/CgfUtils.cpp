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
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    namespace RC
    {
        void ConfigureCgfContent(CContentCGF& content)
        {
            CExportInfoCGF* exportInfo = content.GetExportInfo();

            bool useCustomNormalDefault = true;
            AZ::SceneAPI::Events::AssetImportRequestBus::Broadcast(&AZ::SceneAPI::Events::AssetImportRequestBus::Events::AreCustomNormalsUsed, useCustomNormalDefault);

            exportInfo->bMergeAllNodes = true;
            exportInfo->bUseCustomNormals = useCustomNormalDefault;  // This will be overritten by StaticMeshAdvancedRule (if teh rule exists) when calling ContainerSettingsExporter::ProcessContext
            exportInfo->bCompiledCGF = false;
            exportInfo->bHavePhysicsProxy = false;
            exportInfo->bHaveAutoLods = false;
            exportInfo->bNoMesh = true;
            exportInfo->b8WeightsPerVertex = false;
            exportInfo->bWantF32Vertices = false;
            exportInfo->authorToolVersion = 1;
        }

        void ProcessMeshType(ContainerExportContext& context, CContentCGF& content, const AZStd::vector<AZStd::string>& targetNodes, EPhysicsGeomType physicalizeType)
        {
            const SceneAPI::Containers::SceneGraph& graph = context.m_scene.GetGraph();
            for (const AZStd::string& nodeName : targetNodes)
            {
                AZ_TraceContext("Mesh node", nodeName);
                SceneAPI::Containers::SceneGraph::NodeIndex index = graph.Find(nodeName);
                if (index.IsValid())
                {
                    CNodeCGF* node = new CNodeCGF(); // will be auto deleted by CContentCGF cgf
                    SetNodeName(nodeName, *node);
                    AZStd::string rootBoneName;
                    SceneAPI::Events::Process<NodeExportContext>(context, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Construction);
                    SceneAPI::Events::Process<NodeExportContext>(context, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Filling);
                    content.AddNode(node);
                    SceneAPI::Events::Process<NodeExportContext>(context, *node, nodeName, index, physicalizeType, rootBoneName, Phase::Finalizing);
                }
            }
        }

        void SetNodeName(const AZStd::string& name, CNodeCGF& node)
        {
            static const size_t nodeNameCount = sizeof(node.name) / sizeof(node.name[0]);
            size_t offset = name.length() < nodeNameCount ? 0 : name.length() - nodeNameCount + 1;
            azstrcpy(node.name, nodeNameCount, name.c_str() + offset);
        }

        SceneAPI::Events::ProcessingResult ProcessMeshes(CgfGroupExportContext& context, CContentCGF& content, const AZStd::vector<AZStd::string>& targetNodes)
        {
            SceneAPI::Events::ProcessingResultCombiner result;

            ContainerExportContext containerContext(context.m_scene, context.m_outputDirectory, context.m_group, content, Phase::Construction);
            result += SceneAPI::Events::Process(containerContext);
            result += SceneAPI::Events::Process<ContainerExportContext>(containerContext, Phase::Filling);

            const SceneAPI::Containers::SceneGraph& graph = context.m_scene.GetGraph();

            ProcessMeshType(containerContext, content, targetNodes, PHYS_GEOM_TYPE_NONE);

            result += SceneAPI::Events::Process<ContainerExportContext>(containerContext, Phase::Finalizing);

            return result.GetResult();
        }
    } // RC
} // AZ
