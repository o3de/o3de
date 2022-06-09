/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QStyledItemDelegate>
#include <QPointer>
namespace AssetProcessor
{
    class ProductAssetDetailsPanel;
    class ProductDependencyTreeDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        ProductDependencyTreeDelegate(QObject* parent, QPointer<ProductAssetDetailsPanel> panel);
    protected:
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    private:
        QPointer<ProductAssetDetailsPanel> m_panel;
    };
} // namespace AssetProcessor
