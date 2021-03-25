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

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiHandlerBase.h>

namespace AzToolsFramework
{
    class LayerUiHandler
        : public EditorEntityUiHandlerBase
    {
    public:
        AZ_CLASS_ALLOCATOR(LayerUiHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AzToolsFramework::LayerUiHandler, "{C078A6D9-4E1E-4431-9B6A-859477748BDB}", EditorEntityUiHandlerBase);

        LayerUiHandler() = default;
        ~LayerUiHandler() override = default;

        // EditorEntityUiHandler...
        QString GenerateItemInfoString(AZ::EntityId entityId) const override;
        QPixmap GenerateItemIcon(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;
        void PaintItemForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;

    private:
        static void PaintLayerStripeAndBorder(QPainter* painter, int stripeX, int top, int bottom, QColor layerBorderColor, QColor layerColor);

        static bool IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child);
        static QModelIndex GetLastVisibleChild(const QModelIndex& parent);
        static QModelIndex Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index);

        static const int m_layerSquareSize;
        static const int m_layerStripeWidth;
        static const int m_layerDividerLineHeight;
        static const int m_lastEntityInLayerDividerLineHeight;
        static const QColor m_layerBackgroundColor;
        static const QColor m_layerDescendantBackgroundColor;
        static const QColor m_layerBorderTopColor;
        static const QColor m_layerBorderBottomColor;
        static const QString m_layerIconPath;
    };
} // namespace AzToolsFramework
