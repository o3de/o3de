/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    EditorEntityUiHandlerBase::EditorEntityUiHandlerBase()
    {
        auto editorEntityUiInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();

        AZ_Assert((editorEntityUiInterface != nullptr),
            "Editor Entity Ui Handlers require EditorEntityUiInterface on construction.");

        m_handlerId = editorEntityUiInterface->RegisterHandler(this);
    }

    EditorEntityUiHandlerBase::~EditorEntityUiHandlerBase()
    {
        auto editorEntityUiInterface = AZ::Interface<AzToolsFramework::EditorEntityUiInterface>::Get();

        if (editorEntityUiInterface != nullptr)
        {
            editorEntityUiInterface->UnregisterHandler(this);
        }
    }

    EditorEntityUiHandlerId EditorEntityUiHandlerBase::GetHandlerId()
    {
        return m_handlerId;
    }
    
    QString EditorEntityUiHandlerBase::GenerateItemInfoString([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QString();
    }

    QString EditorEntityUiHandlerBase::GenerateItemTooltip([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QString();
    }

    QIcon EditorEntityUiHandlerBase::GenerateItemIcon([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QIcon();
    }

    bool EditorEntityUiHandlerBase::CanToggleLockVisibility([[maybe_unused]] AZ::EntityId entityId) const
    {
        return true;
    }

    bool EditorEntityUiHandlerBase::CanRename([[maybe_unused]] AZ::EntityId entityId) const
    {
        return true;
    }

    void EditorEntityUiHandlerBase::PaintItemBackground(
        [[maybe_unused]] QPainter* painter,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantBackground(
        [[maybe_unused]] QPainter* painter,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index,
        [[maybe_unused]] const QModelIndex& descendantIndex) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantBranchBackground(
        [[maybe_unused]] QPainter* painter,
        [[maybe_unused]] const QTreeView* view,
        [[maybe_unused]] const QRect& rect,
        [[maybe_unused]] const QModelIndex& index,
        [[maybe_unused]] const QModelIndex& descendantIndex) const
    {
    }

    void EditorEntityUiHandlerBase::PaintItemForeground(
        [[maybe_unused]] QPainter* painter,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantForeground(
        [[maybe_unused]] QPainter* painter,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index,
        [[maybe_unused]] const QModelIndex& descendantIndex) const
    {
    }

    bool EditorEntityUiHandlerBase::OnOutlinerItemClick(
        [[maybe_unused]] const QPoint& position,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
        return false;
    }

    void EditorEntityUiHandlerBase::OnOutlinerItemExpand([[maybe_unused]] const QModelIndex& index) const
    {
    }
    
    void EditorEntityUiHandlerBase::OnOutlinerItemCollapse([[maybe_unused]] const QModelIndex& index) const
    {
    }

    bool EditorEntityUiHandlerBase::OnEntityDoubleClick([[maybe_unused]] AZ::EntityId entityId) const
    {
        return false;
    }

} // namespace AzToolsFramework
