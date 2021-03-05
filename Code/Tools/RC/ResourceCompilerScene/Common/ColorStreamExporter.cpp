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

#include <Cry_Geo.h> // Needed for IIndexedMesh.h
#include <IIndexedMesh.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/ColorStreamExporter.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneUtilities = AZ::SceneAPI::Utilities;

        ColorStreamExporter::ColorStreamExporter()
        {
            BindToCall(&ColorStreamExporter::CopyVertexColorStream);
        }
        
        void ColorStreamExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ColorStreamExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult ColorStreamExporter::CopyVertexColorStream(MeshNodeExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            const SceneDataTypes::IGroup& group = context.m_group;

            AZStd::shared_ptr<const SceneDataTypes::IMeshVertexColorData> colors = nullptr;
            AZStd::string streamName;

            AZStd::shared_ptr<const SceneDataTypes::IMeshAdvancedRule> rule = group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IMeshAdvancedRule>();
            if (!rule || rule->IsVertexColorStreamDisabled() || rule->GetVertexColorStreamName().empty())
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZ_TraceContext("Vertex color stream", rule->GetVertexColorStreamName());
            SceneContainers::SceneGraph::NodeIndex index = graph.Find(context.m_nodeIndex, rule->GetVertexColorStreamName());
            colors = azrtti_cast<const SceneDataTypes::IMeshVertexColorData*>(graph.GetNodeContent(index));
            if (colors)
            {
                bool countMatch = context.m_mesh.GetVertexCount() == colors->GetCount();
                if (!countMatch)
                {
                    AZ_TracePrintf(SceneUtilities::ErrorWindow,
                        "Number of vertices in the mesh (%i) don't match with the number of stored vertex color stream (%i).",
                        context.m_mesh.GetVertexCount(), colors->GetCount());
                    return SceneEvents::ProcessingResult::Failure;
                }

                // Vertex coloring always uses the first vertex color stream.
                context.m_mesh.ReallocStream(CMesh::COLORS, 0, context.m_mesh.GetVertexCount());
                for (int i = 0; i < context.m_mesh.GetVertexCount(); ++i)
                {
                    const SceneDataTypes::Color& color = colors->GetColor(i);
                    context.m_mesh.m_pColor0[i] = SMeshColor(
                        static_cast<uint8_t>(GetClamp<float>(color.red, 0.0f, 1.0f) * 255.0f),
                        static_cast<uint8_t>(GetClamp<float>(color.green, 0.0f, 1.0f) * 255.0f),
                        static_cast<uint8_t>(GetClamp<float>(color.blue, 0.0f, 1.0f) * 255.0f),
                        static_cast<uint8_t>(GetClamp<float>(color.alpha, 0.0f, 1.0f) * 255.0f));
                }
            }
            else
            {
                AZ_TracePrintf(SceneUtilities::WarningWindow, "Vertex color stream not found or name doesn't refer to a vertex color stream.");
                context.m_mesh.ReallocStream(CMesh::COLORS, 0, context.m_mesh.GetVertexCount());
                for (int i = 0; i < context.m_mesh.GetVertexCount(); ++i)
                {
                    context.m_mesh.m_pColor0[i] = SMeshColor(255, 255, 255, 255);
                }
            }
            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace RC
} // namespace AZ