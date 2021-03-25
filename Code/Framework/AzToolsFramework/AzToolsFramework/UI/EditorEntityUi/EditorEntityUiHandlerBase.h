/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiInterface.h>

#include <QPixmap>
#include <QStyleOptionViewItem>

class QPainter;

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

        //! Paints the background of the item in the Outliner.
        virtual void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        //! Paints the background of the descendants of the item in the Outliner.
        virtual void PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const;

        //! Paints visual elements on the foreground of the item in the Outliner.
        virtual void PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
        //! Paints visual elements on the foreground of the descendants of the item in the Outliner.
        virtual void PaintDescendantForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const;

    private:
        EditorEntityUiHandlerId m_handlerId = 0;
    };
} // namespace AzToolsFramework
