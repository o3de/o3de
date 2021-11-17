/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QColor>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QIcon>
AZ_POP_DISABLE_WARNING

namespace GraphCanvas
{
    ////////////////////////
    // NodePaletteTreeItem
    ////////////////////////

    const int NodePaletteTreeItem::k_defaultItemOrdering = 100;

    NodePaletteTreeItem::NodePaletteTreeItem(AZStd::string_view name, EditorId editorId)
        : GraphCanvas::GraphCanvasTreeItem()
        , m_editorId(editorId)
        , m_name(QString::fromUtf8(name.data(), static_cast<int>(name.size())))
        , m_selected(false)
        , m_hovered(false)
        , m_enabled(true)
        , m_highlight(-1, 0)
        , m_ordering(k_defaultItemOrdering)
    {
    }

    const QString& NodePaletteTreeItem::GetName() const
    {
        return m_name;
    }

    int NodePaletteTreeItem::GetColumnCount() const
    {
        return Column::Count;
    }

    QVariant NodePaletteTreeItem::Data(const QModelIndex& index, int role) const
    {
        if (index.column() == Column::Name)
        {
            switch (role)
            {
            case Qt::ToolTipRole:
                if (HasError())
                {
                    return m_errorString;
                }
                else
                {
                    // If we have a tooltip. Use it
                    // Otherwise fall through to use our name.
                    if (!m_toolTip.isEmpty())
                    {
                        return m_toolTip;
                    }
                }
                break;
            case Qt::DisplayRole:
                return GetName();
            case Qt::EditRole:
                return GetName();
            case Qt::ForegroundRole:
                if (!IsEnabled())
                {
                    QVariant variant = OnData(index, role);

                    if (variant.type() == QVariant::Type::Color)
                    {
                        QColor fontColor = variant.value<QColor>();

                        int fontAlpha = aznumeric_cast<int>(fontColor.alpha() * 0.5f);
                        fontAlpha = AZStd::min(AZStd::min(fontAlpha, 127), fontColor.alpha());

                        fontColor.setAlpha(fontAlpha);

                        variant.setValue(fontColor);
                    }

                    return variant;
                }
                break;
            case Qt::DecorationRole:
                if (HasError())
                {
                    return QIcon(":/GraphCanvasEditorResources/toast_error_icon.png");
                }
                break;
            default:
                break;
            }
        }

        return OnData(index, role);
    }

    Qt::ItemFlags NodePaletteTreeItem::Flags(const QModelIndex& /*index*/) const
    {
        Qt::ItemFlags baseFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        return baseFlags | OnFlags();
    }

    void NodePaletteTreeItem::SetToolTip(const QString& toolTip)
    {
        m_toolTip = toolTip;
    }

    void NodePaletteTreeItem::SetItemOrdering(int ordering)
    {
        m_ordering = ordering;
        SignalLayoutChanged();
    }

    void NodePaletteTreeItem::SetStyleOverride(const AZStd::string& styleOverride)
    {
        m_styleOverride = styleOverride;

        if (!m_styleOverride.empty())
        {
            for (int i = 0; i < GetChildCount(); ++i)
            {
                NodePaletteTreeItem* childItem = static_cast<NodePaletteTreeItem*>(FindChildByRow(i));

                childItem->SetStyleOverride(styleOverride);
            }
        }

        OnStyleOverrideChange();
    }

    const AZStd::string& NodePaletteTreeItem::GetStyleOverride() const
    {
        return m_styleOverride;
    }

    void NodePaletteTreeItem::SetTitlePalette(const AZStd::string& palette, bool force)
    {
        if (force || m_palette.empty() || m_palette.compare(DefaultNodeTitlePalette) == 0)
        {
            m_palette = palette;

            if (!m_palette.empty())
            {
                for (int i = 0; i < GetChildCount(); ++i)
                {
                    NodePaletteTreeItem* childItem = static_cast<NodePaletteTreeItem*>(FindChildByRow(i));

                    childItem->SetTitlePalette(palette);
                }
            }

            OnTitlePaletteChanged();
        }
    }

    const AZStd::string& NodePaletteTreeItem::GetTitlePalette() const
    {
        if (IsEnabled())
        {
            return m_palette;
        }
        else
        {
            static AZStd::string s_disabledPalette = "DisabledPalette";
            return s_disabledPalette;
        }
    }

    void NodePaletteTreeItem::SetHovered(bool hovered)
    {
        if (m_hovered != hovered)
        {
            m_hovered = hovered;
            OnHoverStateChanged();
        }
    }

    bool NodePaletteTreeItem::IsHovered() const
    {
        return m_hovered;
    }

    void NodePaletteTreeItem::SetSelected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            OnSelectionStateChanged();
        }
    }

    bool NodePaletteTreeItem::IsSelected() const
    {
        return m_selected;
    }

    void NodePaletteTreeItem::SetEnabled(bool enabled)
    {
        if (m_enabled != enabled)
        {
            m_enabled = enabled;
            OnTitlePaletteChanged();
            OnEnabledStateChanged();
            SignalDataChanged();
        }
    }

    bool NodePaletteTreeItem::IsEnabled() const
    {
        return m_enabled;
    }

    void NodePaletteTreeItem::SetHighlight(const AZStd::pair<int, int>& highlight)
    {
        m_highlight = highlight;
    }

    bool NodePaletteTreeItem::HasHighlight() const
    {
        return m_highlight.first >= 0 && m_highlight.second > 0;
    }

    const AZStd::pair<int, int>& NodePaletteTreeItem::GetHighlight() const
    {
        return m_highlight;
    }

    void NodePaletteTreeItem::ClearHighlight()
    {
        m_highlight.first = -1;
        m_highlight.second = 0;
    }

    void NodePaletteTreeItem::SignalClicked(int row)
    {
        OnClicked(row);
    }

    bool NodePaletteTreeItem::SignalDoubleClicked(int row)
    {
        return OnDoubleClicked(row);
    }

    void NodePaletteTreeItem::SetError(const AZStd::string& errorString)
    {
        m_errorString = errorString.c_str();
        SignalDataChanged();
    }

    void NodePaletteTreeItem::ClearError()
    {
        SetError("");
    }

    bool NodePaletteTreeItem::HasError() const
    {
        return !m_errorString.isEmpty();
    }

    void NodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        if (!m_styleOverride.empty())
        {
            static_cast<NodePaletteTreeItem*>(item)->SetStyleOverride(m_styleOverride);
        }

        if (!m_palette.empty())
        {
            static_cast<NodePaletteTreeItem*>(item)->SetTitlePalette(m_palette);
        }
    }

    void NodePaletteTreeItem::SetName(const QString& name)
    {
        m_name = name;
        SignalDataChanged();
    }

    const EditorId& NodePaletteTreeItem::GetEditorId() const
    {
        return m_editorId;
    }

    bool NodePaletteTreeItem::LessThan(const GraphCanvasTreeItem* graphItem) const
    {
        const NodePaletteTreeItem* otherItem = static_cast<const NodePaletteTreeItem*>(graphItem);
        if (m_ordering == otherItem->m_ordering)
        {
            return m_name < static_cast<const NodePaletteTreeItem*>(graphItem)->m_name;
        }
        else
        {
            return m_ordering < otherItem->m_ordering;
        }
    }

    QVariant NodePaletteTreeItem::OnData(const QModelIndex& /*index*/, int /*role*/) const
    {
        return QVariant();
    }

    Qt::ItemFlags NodePaletteTreeItem::OnFlags() const
    {
        return Qt::ItemFlags();
    }

    void NodePaletteTreeItem::OnStyleOverrideChange()
    {
    }

    void NodePaletteTreeItem::OnTitlePaletteChanged()
    {
    }

    void NodePaletteTreeItem::OnHoverStateChanged()
    {

    }

    void NodePaletteTreeItem::OnSelectionStateChanged()
    {

    }

    void NodePaletteTreeItem::OnEnabledStateChanged()
    {

    }

    void NodePaletteTreeItem::OnClicked(int row)
    {
        AZ_UNUSED(row);
    }

    bool NodePaletteTreeItem::OnDoubleClicked(int row)
    {
        AZ_UNUSED(row);
        return false;
    }
}
