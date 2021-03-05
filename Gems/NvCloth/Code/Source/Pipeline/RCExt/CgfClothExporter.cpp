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

#include <AzToolsFramework/Debug/TraceContext.h>

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Color.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>

#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>

#include <Pipeline/RCExt/CgfClothExporter.h>

namespace NvCloth
{
    namespace Pipeline
    {
        namespace
        {
            // Index for the Vertex color stream that contains the cloth inverse masses.
            const int ClothVertexBufferStreamIndex = 1;
        }

        CgfClothExporter::CgfClothExporter()
        {
            // Binding the processing functions so when exporters call
            // SceneAPI::Events::Process<Context>() these functions will
            // get called if their Context was used.
            BindToCall(&CgfClothExporter::ProcessMeshNodeContext);
            BindToCall(&CgfClothExporter::ProcessContainerContext);
        }

        void CgfClothExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<CgfClothExporter, AZ::SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult CgfClothExporter::ProcessContainerContext(AZ::RC::ContainerExportContext& context) const
        {
            if (!context.m_group.GetRuleContainerConst().ContainsRuleOfType<AZ::SceneAPI::DataTypes::IClothRule>())
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            if (context.m_phase == AZ::RC::Phase::Finalizing)
            {
                if (context.m_container.GetExportInfo()->bMergeAllNodes)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow,
                        "Mesh group '%s' has cloth rules and trying to merge all nodes.",
                        context.m_group.GetName().c_str());
                    return AZ::SceneAPI::Events::ProcessingResult::Failure;
                }
            }
            else
            {
                // If the current mesh group contains a cloth rule it should not merge all the nodes.
                context.m_container.GetExportInfo()->bMergeAllNodes = false;
            }

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        AZ::SceneAPI::Events::ProcessingResult CgfClothExporter::ProcessMeshNodeContext(AZ::RC::MeshNodeExportContext& context) const
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            AZStd::vector<AZ::Color> clothData =
                AZ::SceneAPI::DataTypes::IClothRule::FindClothData(
                    context.m_scene.GetGraph(),
                    context.m_nodeIndex,
                    static_cast<size_t>(context.m_mesh.GetVertexCount()),
                    context.m_group.GetRuleContainerConst());

            if (!clothData.empty())
            {
                const int numVertices = context.m_mesh.GetVertexCount();

                // Allocate and get the vertex color stream for cloth
                context.m_mesh.ReallocStream(CMesh::COLORS, ClothVertexBufferStreamIndex, numVertices);
                auto meshColorStream = context.m_mesh.GetStreamPtr<SMeshColor>(CMesh::COLORS, ClothVertexBufferStreamIndex);
                AZ_Assert(meshColorStream, "Mesh color stream is invalid");

                for (int i = 0; i < numVertices; ++i)
                {
                    const auto& clothVertexData = clothData[i];
                    meshColorStream[i] = SMeshColor(
                        clothVertexData.GetR8(),
                        clothVertexData.GetG8(),
                        clothVertexData.GetB8(),
                        clothVertexData.GetA8());
                }
            }

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }
    } // namespace Pipeline
} // namespace NvCloth
