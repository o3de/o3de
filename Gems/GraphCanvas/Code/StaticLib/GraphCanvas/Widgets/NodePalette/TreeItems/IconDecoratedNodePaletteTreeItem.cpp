/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <qpixmap.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Components/StyleBus.h>

namespace GraphCanvas
{
    /////////////////////////////////////
    // IconDecoratedNodePaletteTreeItem
    /////////////////////////////////////
    IconDecoratedNodePaletteTreeItem::IconDecoratedNodePaletteTreeItem(AZStd::string_view name, EditorId editorId)
        : NodePaletteTreeItem(name, editorId)
    {
        m_paletteConfiguration.m_iconPalette = "NodePaletteTypeIcon";
        SetTitlePalette(NodePaletteTreeItem::DefaultNodeTitlePalette);

        StyleManagerNotificationBus::Handler::BusConnect(editorId);

        // We want anything with icons on it to be group together(since in theory, the non-icon versions will be folders)
        SetItemOrdering(k_defaultItemOrdering - 1);
    }

    void IconDecoratedNodePaletteTreeItem::AddIconColorPalette(const AZStd::string& colorPalette)
    {
        m_paletteConfiguration.AddColorPalette(colorPalette);
        StyleManagerRequestBus::EventResult(m_iconPixmap, GetEditorId(), &StyleManagerRequests::GetConfiguredPaletteIcon, m_paletteConfiguration);
    }

    void IconDecoratedNodePaletteTreeItem::OnStylesUnloaded()
    {
        m_iconPixmap = nullptr;
    }

    void IconDecoratedNodePaletteTreeItem::OnStylesLoaded()
    {
        StyleManagerRequestBus::EventResult(m_iconPixmap, GetEditorId(), &StyleManagerRequests::GetConfiguredPaletteIcon, m_paletteConfiguration);
    }

    QVariant IconDecoratedNodePaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        if (index.column() == NodePaletteTreeItem::Column::Name)
        {
            switch (role)
            {
            case Qt::DecorationRole:
                if (m_iconPixmap != nullptr)
                {
                    return (*m_iconPixmap);
                }
            default:
                return QVariant();
            }
        }
        else
        {
            return NodePaletteTreeItem::OnData(index, role);
        }
    }

    void IconDecoratedNodePaletteTreeItem::OnTitlePaletteChanged()
    {
        // Need to come up with a better way of dealing with the multi-state title palettes
        m_paletteConfiguration.SetColorPalette(GetTitlePalette());
        StyleManagerRequestBus::EventResult(m_iconPixmap, GetEditorId(), &StyleManagerRequests::GetConfiguredPaletteIcon, m_paletteConfiguration);
    }
}
