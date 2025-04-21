/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>
#include <GraphModel/Integration/NodePalette/ModuleNodePaletteItem.h>
#include <AzCore/IO/Path/Path.h>

namespace GraphModelIntegration
{
    /// Add common utilities to a specific Node Palette tree.
    void AddCommonNodePaletteUtilities(GraphCanvas::GraphCanvasTreeItem* rootItem, const GraphCanvas::EditorId& editorId)
    {
        GraphCanvas::IconDecoratedNodePaletteTreeItem* utilitiesCategory = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Utilities", editorId);
        utilitiesCategory->SetTitlePalette("UtilityNodeTitlePalette");
        utilitiesCategory->CreateChildNode<CommentNodePaletteTreeItem>("Comment", editorId);
        utilitiesCategory->CreateChildNode<NodeGroupNodePaletteTreeItem>("Node Group", editorId);
    }

    AZStd::string GetNodeName(AZStd::string_view sourceFileName)
    {
        AZ::IO::PathView name = AZ::IO::PathView(sourceFileName).Filename();
        if (name.empty())
        {
            name = "unnamed";
            AZ_Assert(false, "Could not get node name from module file path [%.*s]", AZ_STRING_ARG(sourceFileName));
        }
        return name.String();
    }

    void CreateGraphCanvasNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGraphCanvasNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    void CreateCommentNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCommentNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    void CreateNodeGroupNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeGroupNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    void CreateModuleNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateModuleNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("m_sourceFileName", &CreateModuleNodeMimeEvent::m_sourceFileName)
                ->Field("m_sourceFileId", &CreateModuleNodeMimeEvent::m_sourceFileId)
                ;
        }
    }
}
