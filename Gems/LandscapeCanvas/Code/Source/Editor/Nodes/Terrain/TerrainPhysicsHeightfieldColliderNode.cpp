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
#include <Editor/Nodes/Terrain/TerrainPhysicsHeightfieldColliderNode.h>

namespace LandscapeCanvas
{
    void TerrainPhysicsHeightfieldColliderNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainPhysicsHeightfieldColliderNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainPhysicsHeightfieldColliderNode>("TerrainPhysicsHeightfieldColliderNode", "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainPhysicsHeightfieldColliderNode::TITLE = "Terrain Heightfield Physics Collider";

    TerrainPhysicsHeightfieldColliderNode::TerrainPhysicsHeightfieldColliderNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const BaseNode::BaseNodeType TerrainPhysicsHeightfieldColliderNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainExtender;
    }

    const char* TerrainPhysicsHeightfieldColliderNode::GetTitle() const
    {
        return TITLE;
    }

    AZ::ComponentDescriptor::DependencyArrayType TerrainPhysicsHeightfieldColliderNode::GetOptionalRequiredServices() const
    {
        return AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("PhysicsHeightfieldColliderService") };
    }
} // namespace LandscapeCanvas
