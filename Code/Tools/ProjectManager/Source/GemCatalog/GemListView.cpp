/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemItemDelegate.h>
#include <QStandardItemModel>
#include <QProxyStyle>

namespace O3DE::ProjectManager
{
    class GemListViewProxyStyle : public QProxyStyle
    {
    public:
        using QProxyStyle::QProxyStyle;
        int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
        {
            if (hint == QStyle::SH_ToolTip_WakeUpDelay || hint == QStyle::SH_ToolTip_FallAsleepDelay)
            {
                // no delay
                return 0;
            }

            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    };

    GemListView::GemListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent)
        : QListView(parent)
    {
        setObjectName("GemCatalogListView");
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        setModel(model);
        setSelectionModel(selectionModel);
        setItemDelegate(new GemItemDelegate(model, this));

        // use a custom proxy style so we get immediate tooltips for gem radio buttons
        setStyle(new GemListViewProxyStyle(this->style()));
    }
} // namespace O3DE::ProjectManager
