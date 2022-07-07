/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        QIcon GenerateItemIcon(AZ::EntityId entityId) const override;
        void PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void PaintDescendantBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;
        void PaintDescendantBranchBackground(QPainter* painter, const QTreeView* view, const QRect& rect,
            const QModelIndex& index, const QModelIndex& descendantIndex) const override;
        void PaintDescendantForeground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            const QModelIndex& descendantIndex) const override;

    private:
        static void PaintLayerStripeAndBorder(QPainter* painter, int stripeX, int top, int bottom, QColor layerBorderColor, QColor layerColor);

        static bool IsLastVisibleChild(const QModelIndex& parent, const QModelIndex& child);
        static QModelIndex GetLastVisibleChild(const QModelIndex& parent);
        static QModelIndex Internal_GetLastVisibleChild(const QAbstractItemModel* model, const QModelIndex& index);

        static constexpr int m_layerSquareSize = 22;
        static constexpr int m_layerStripeWidth = 1;
        static constexpr int m_layerDividerLineHeight = 1;
        static constexpr int m_lastEntityInLayerDividerLineHeight = 1;
        static const QColor m_layerBackgroundColor;
        static const QColor m_layerBackgroundHoveredColor;
        static const QColor m_layerBackgroundSelectedColor;
        static const QColor m_layerDescendantBackgroundColor;
        static const QColor m_layerDescendantBackgroundHoveredColor;
        static const QColor m_layerDescendantBackgroundSelectedColor;
        static const QColor m_layerBorderTopColor;
        static const QColor m_layerBorderBottomColor;
        static const QString m_layerIconPath;
    };
} // namespace AzToolsFramework
