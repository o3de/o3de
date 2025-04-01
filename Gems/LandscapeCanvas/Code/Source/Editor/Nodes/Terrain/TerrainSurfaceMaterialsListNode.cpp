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
#include <Editor/Nodes/Terrain/TerrainSurfaceMaterialsListNode.h>

namespace LandscapeCanvas
{
    void TerrainSurfaceMaterialsListNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainSurfaceMaterialsListNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainSurfaceMaterialsListNode>("TerrainSurfaceMaterialsListNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainSurfaceMaterialsListNode::TITLE = "Terrain Surface Materials List";

    TerrainSurfaceMaterialsListNode::TerrainSurfaceMaterialsListNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType TerrainSurfaceMaterialsListNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainExtender;
    }

    const char* TerrainSurfaceMaterialsListNode::GetTitle() const
    {
        return TITLE;
    }

    void TerrainSurfaceMaterialsListNode::RegisterSlots()
    {
        CreateEntityNameSlot();
    }
} // namespace LandscapeCanvas
