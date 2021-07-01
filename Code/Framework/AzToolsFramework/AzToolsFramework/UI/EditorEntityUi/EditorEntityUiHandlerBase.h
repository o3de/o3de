/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>

#include <QPixmap>
#include <QStyleOptionViewItem>

class QPainter;
class QTreeView;

namespace AzToolsFramework
{
    //! Defines a handler that can customize entity UI appearance and behavior in the Entity Outliner.
    //! This class is meant to be abstract, entities do not have a handler by default.
    class EditorEntityUiHandlerBase
    {
    protected:
        AZ_RTTI(EditorEntityUiHandlerBase, "{EFB6CBC0-3A0A-4302-92EB-62CAF1C47163}");

        EditorEntityUiHandlerBase();
        virtual ~EditorEntityUiHandlerBase();

    public:
        EditorEntityUiHandlerId GetHandlerId();

        // # Entity Outliner

        //! Returns the item info string that is appended to the item name in the Outliner.
        virtual QString GenerateItemInfoString(AZ::EntityId entityId) const;
        //! Returns the item tooltip text to display in the Outliner.
        virtual QString GenerateItemTooltip(AZ::EntityId entityId) const;
        //! Returns the item icon pixmap to display in the Outliner.
        virtual QPixmap GenerateItemIcon(AZ::EntityId entityId) const;
        //! Returns whether the element's lock and visibility state should be accessible in the Outliner
        virtual bool CanToggleLockVisibility(AZ::EntityId entityId) const;
        //! Returns whether the element's name should be editable
        virtual bool CanRename(AZ::EntityId entityId) const;

        //! Paints the background of the item in the Outliner.
        virtual void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        //! Paints the background of the descendants of the item in the Outliner.
        virtual void PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const;
        //! Paints the background of the descendant branches of the item in the Outliner.
        virtual void PaintDescendantBranchBackground(QPainter* painter, const QTreeView* view, const QRect& rect,
            const QModelIndex& index, const QModelIndex& descendantIndex) const;

        //! Paints visual elements on the foreground of the item in the Outliner.
        virtual void PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        //! Paints visual elements on the foreground of the descendants of the item in the Outliner.
        virtual void PaintDescendantForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const;

    private:
        EditorEntityUiHandlerId m_handlerId = 0;
    };
} // namespace AzToolsFramework
