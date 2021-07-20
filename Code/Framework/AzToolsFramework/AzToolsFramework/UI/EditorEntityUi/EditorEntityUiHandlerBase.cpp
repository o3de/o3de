/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    
    QString EditorEntityUiHandlerBase::GenerateItemInfoString(AZ::EntityId /*entityId*/) const
    {
        return QString();
    }

    QString EditorEntityUiHandlerBase::GenerateItemTooltip(AZ::EntityId /*entityId*/) const
    {
        return QString();
    }

    QPixmap EditorEntityUiHandlerBase::GenerateItemIcon(AZ::EntityId /*entityId*/) const
    {
        return QPixmap();
    }

    bool EditorEntityUiHandlerBase::CanToggleLockVisibility(AZ::EntityId /*entityId*/) const
    {
        return true;
    }

    bool EditorEntityUiHandlerBase::CanRename(AZ::EntityId /*entityId*/) const
    {
        return true;
    }

    void EditorEntityUiHandlerBase::PaintItemBackground(QPainter* /*painter*/, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantBackground(QPainter* /*painter*/, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/,
        const QModelIndex& /*descendantIndex*/) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantBranchBackground(QPainter* /*painter*/, const QTreeView* /*view*/, const QRect& /*rect*/,
        const QModelIndex& /*index*/, const QModelIndex& /*descendantIndex*/) const
    {
    }

    void EditorEntityUiHandlerBase::PaintItemForeground(QPainter* /*painter*/, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
    {
    }

    void EditorEntityUiHandlerBase::PaintDescendantForeground(QPainter* /*painter*/, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/,
        const QModelIndex& /*descendantIndex*/) const
    {
    }

} // namespace AzToolsFramework
