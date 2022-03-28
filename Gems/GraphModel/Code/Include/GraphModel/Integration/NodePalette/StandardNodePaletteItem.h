/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Canvas
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

// Graph Model
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Integration/Helpers.h>

namespace GraphModelIntegration
{
    template<typename NodeType>
    class CreateStandardNodeMimeEvent;

    //! Provides a common interface for instantiating GraphModel::Node subclasses through the Node Palette
    template<typename NodeType>
    class StandardNodePaletteItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(StandardNodePaletteItem, AZ::SystemAllocator, 0);

        //! Constructor
        //! \param nodeName Name of the node that will show up in the Palette
        //! \param editorId Unique name of the client system editor (ex: AZ_CRC("ShaderCanvas", 0x0a1dff96))
        StandardNodePaletteItem(AZStd::string_view nodeName, GraphCanvas::EditorId editorId)
            : DraggableNodePaletteTreeItem(nodeName, editorId)
        {
            // Setting the palette override (if specified) is mainly used to set the icon color for this
            // node palette item, but it can also be used to override other styling aspects as well
            AZStd::string paletteOverride = Helpers::GetTitlePaletteOverride(azrtti_typeid<NodeType>());
            if (!paletteOverride.empty())
            {
                SetTitlePalette(paletteOverride);
            }
        }
        ~StandardNodePaletteItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override
        {
            return aznew CreateStandardNodeMimeEvent<NodeType>();
        }
    };

    template<typename NodeType>
    class CreateStandardNodeMimeEvent
        : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI( ( (CreateStandardNodeMimeEvent<NodeType>), "{DF6213A0-5C60-4C22-88F1-4CEA6D8A17EF}", NodeType), GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateStandardNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

            if (serializeContext)
            {
                serializeContext->Class<CreateStandardNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                    ->Version(0)
                    ;
            }
        }
        
        bool ExecuteEvent([[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override
        {
            GraphModel::GraphPtr graph = nullptr;
            GraphManagerRequestBus::BroadcastResult(graph, &GraphManagerRequests::GetGraph, graphCanvasSceneId);
            if (!graph)
            {
                return false;
            }

            AZStd::shared_ptr<GraphModel::Node> node = AZStd::make_shared<NodeType>(graph);
            if (!node)
            {
                return false;
            }

            GraphControllerRequestBus::EventResult(m_createdNodeId, graphCanvasSceneId, &GraphControllerRequests::AddNode, node, dropPosition);

            return true;
        }
    };

    template<typename NodeType>
    void ReflectAndCreateNodeMimeEvent(AZ::ReflectContext* context)
    {
        NodeType::Reflect(context);
        GraphModelIntegration::CreateStandardNodeMimeEvent<NodeType>::Reflect(context);
    }
}
