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

#include "precompiled.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

namespace ScriptCanvas
{
    ScriptCanvasData::ScriptCanvasData(ScriptCanvasData&& other)
        : m_scriptCanvasEntity(AZStd::move(other.m_scriptCanvasEntity))
    {
    }

    ScriptCanvasData& ScriptCanvasData::operator=(ScriptCanvasData&& other)
    {
        m_scriptCanvasEntity = AZStd::move(other.m_scriptCanvasEntity);
        return *this;
    }

    static bool ScriptCanvasDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootDataElementNode)
    {
        if (rootDataElementNode.GetVersion() == 0)
        {
            int scriptCanvasEntityIndex = rootDataElementNode.FindElement(AZ_CRC("m_scriptCanvas", 0xfcd20d85));
            if (scriptCanvasEntityIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Version Converter failed, The Script Canvas Entity is missing");
                return false;
            }

            auto scComponentElements = AZ::Utils::FindDescendantElements(context, rootDataElementNode, AZStd::vector<AZ::Crc32>{AZ_CRC("m_scriptCanvas", 0xfcd20d85),
                AZ_CRC("element", 0x41405e39), AZ_CRC("Components", 0xee48f5fd)});
            if (!scComponentElements.empty())
            {
                scComponentElements.front()->AddElementWithData(context, "element", ScriptCanvasEditor::EditorGraphVariableManagerComponent());
            }
        }

        if (rootDataElementNode.GetVersion() < 4)
        {
            auto scEntityElements = AZ::Utils::FindDescendantElements(context, rootDataElementNode,
                AZStd::vector<AZ::Crc32>{AZ_CRC("m_scriptCanvas", 0xfcd20d85), AZ_CRC("element", 0x41405e39)});
            if (scEntityElements.empty())
            {
                AZ_Error("Script Canvas", false, "Version Converter failed, The Script Canvas Entity is missing");
                return false;
            }
            auto& scEntityDataElement = *scEntityElements.front();

            AZ::Entity scEntity;
            if (!scEntityDataElement.GetData(scEntity))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve entity data from the Data Element");
                return false;
            }

            auto graph = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(&scEntity);
            if (!graph)
            {
                AZ_Error("Script Canvas", false, "Script Canvas graph component could not be found on Script Canvas Entity for ScriptCanvasData version %u", rootDataElementNode.GetVersion());
                return false;
            }
            auto variableManager = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::GraphVariableManagerComponent>(&scEntity);
            if (!variableManager)
            {
                AZ_Error("Script Canvas", false, "Script Canvas variable manager component could not be found on Script Canvas Entity for ScriptCanvasData version %u", rootDataElementNode.GetVersion());
                return false;
            }

            variableManager->ConfigureScriptCanvasId(graph->GetScriptCanvasId());
            if (!scEntityDataElement.SetData(context, scEntity))
            {
                AZ_Error("Script Canvas", false, "Failed to set converted Script Canvas Entity back on data element node when transitioning from version %u to version 4", rootDataElementNode.GetVersion());
                return false;
            }
        }

        return true;
    }


    void ScriptCanvasData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<ScriptCanvasData>()
                ->Version(4, &ScriptCanvasDataVersionConverter)
                ->Field("m_scriptCanvas", &ScriptCanvasData::m_scriptCanvasEntity)
                ;
        }
    }

}

namespace ScriptCanvasEditor
{
    ScriptCanvas::Graph* ScriptCanvasAsset::GetScriptCanvasGraph() const
    {
        return AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(m_data->m_scriptCanvasEntity.get());
    }    

    ScriptCanvas::ScriptCanvasData& ScriptCanvasAsset::GetScriptCanvasData()
    {
        return *m_data;
    }

    const ScriptCanvas::ScriptCanvasData& ScriptCanvasAsset::GetScriptCanvasData() const
    {
        return *m_data;
    }
}
