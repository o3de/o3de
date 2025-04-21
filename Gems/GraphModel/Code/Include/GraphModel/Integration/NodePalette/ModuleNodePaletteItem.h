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
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Module/ModuleNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModelIntegration
{
    AZStd::string GetNodeName(AZStd::string_view sourceFileName);

    class CreateModuleNodeMimeEvent
        : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateModuleNodeMimeEvent, "{914F9D88-7B60-408D-A16F-BCCE4CA41EFB}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateModuleNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateModuleNodeMimeEvent() = default; // required by SerializeContext
        CreateModuleNodeMimeEvent(AZStd::string_view sourceFileName, AZ::Uuid sourceFileId)
            : m_sourceFileName(sourceFileName)
            , m_sourceFileId(sourceFileId)
        {
        }

        bool ExecuteEvent([[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override
        {
            GraphModel::GraphPtr graph = nullptr;
            GraphManagerRequestBus::BroadcastResult(graph, &GraphManagerRequests::GetGraph, graphCanvasSceneId);
            if (!graph)
            {
                return false;
            }

            AZStd::shared_ptr<GraphModel::Node> node = AZStd::make_shared<GraphModel::ModuleNode>(graph, m_sourceFileId, m_sourceFileName);
            if (!node)
            {
                return false;
            }

            GraphControllerRequestBus::Event(graphCanvasSceneId, &GraphControllerRequests::AddNode, node, dropPosition);

            return true;
        }

    protected:
        AZStd::string m_sourceFileName;
        AZ::Uuid m_sourceFileId;
    };

    //! Provides the interface for instantiating ModuleNodes through the Node Palette. The ModuleNode is based on a
    //! module node graph file that defines the inputs, outputs, and behavior of the node.
    class ModuleNodePaletteItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(ModuleNodePaletteItem, AZ::SystemAllocator);

        //! Constructor
        //! \param editorId        Unique name of the client system editor (ex: AZ_CRC_CE("ShaderCanvas"))
        //! \param sourceFileId    The unique id for the module node graph source file.
        //! \param sourceFilePath  The path to the module node graph source file. This will be used for node naming and debug output.
        ModuleNodePaletteItem(GraphCanvas::EditorId editorId, AZ::Uuid sourceFileId, AZStd::string_view sourceFilePath)
            : DraggableNodePaletteTreeItem(GetNodeName(sourceFilePath).data(), editorId)
            , m_sourceFileName(sourceFilePath)
            , m_sourceFileId(sourceFileId)
        {}
        ~ModuleNodePaletteItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override
        {
            return aznew CreateModuleNodeMimeEvent(m_sourceFileName, m_sourceFileId);
        }

    protected:
        AZStd::string m_sourceFileName;
        AZ::Uuid m_sourceFileId;
    };
}
