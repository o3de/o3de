/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/ui/ProductDependencyTreeDelegate.h>
#include <native/ui/ProductDependencyTreeItemData.h>
#include <native/ui/ProductAssetDetailsPanel.h>
#include <native/ui/ProductAssetTreeItemData.h>

#include <QMouseEvent>
#include <QApplication>
#include <AzCore/Debug/Trace.h>
#include <iostream>
namespace AssetProcessor
{
    ProductDependencyTreeDelegate::ProductDependencyTreeDelegate(QObject* parent, QPointer<ProductAssetDetailsPanel> panel)
        : QStyledItemDelegate(parent)
        , m_panel(panel)
    {
    }
    bool ProductDependencyTreeDelegate::editorEvent(
        QEvent* event, [[maybe_unused]] QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QWidget* widget = option.widget;
            QStyle* style;
            style = (widget ? widget->style() : QApplication::style());
            // Currently, the icon area is the leftmost part of SE_ItemViewItemText, instead of SE_ItemViewItemDecoration.
            // As a workaround, I get the icon area by fetching TextRect and set width to be the same as height.
            QRect rect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);
            rect.setWidth(rect.height());
            if (rect.contains(mouseEvent->pos()))
            {
                m_panel->GoToProduct((static_cast<ProductDependencyTreeItem*>(index.internalPointer()))->GetData()->m_productName);
            }
            return true;
        }
        return false;
    }
} // namespace AssetProcessor

