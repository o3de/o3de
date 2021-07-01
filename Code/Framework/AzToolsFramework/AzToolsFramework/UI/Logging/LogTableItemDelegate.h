/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzCore/std/containers/unordered_map.h>
#endif

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogTableItemDelegate
            : public AzQtComponents::TableViewItemDelegate
        {
            Q_OBJECT

        public:
            using AzQtComponents::TableViewItemDelegate::TableViewItemDelegate;

            QRect itemViewItemRect(const AzQtComponents::Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const AzQtComponents::TableView::Config& config) override;

            QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

            void geometriesUpdating(AzQtComponents::TableView* tableView) override;

        protected:
            void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
            void clearCachedValues() override;

        private:
            mutable AZStd::unordered_map<int, QSize> m_cachedSizes;
        };
    } // namespace Logging
} // namespace AzToolsFramework
