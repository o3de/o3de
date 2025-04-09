/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QObject>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphModel/Integration/Helpers.h>

#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Terrain/TerrainMacroMaterialNode.h>

namespace LandscapeCanvas
{
    void TerrainMacroMaterialNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainMacroMaterialNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainMacroMaterialNode>("TerrainMacroMaterialNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainMacroMaterialNode::TITLE = "Terrain Macro Material";

    TerrainMacroMaterialNode::TerrainMacroMaterialNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType TerrainMacroMaterialNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainExtender;
    }

    const char* TerrainMacroMaterialNode::GetTitle() const
    {
        return TITLE;
    }

    void TerrainMacroMaterialNode::RegisterSlots()
    {
        CreateEntityNameSlot();
    }
} // namespace LandscapeCanvas
