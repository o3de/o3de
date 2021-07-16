/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Logging/LogTableItemDelegate.h>
#include <AzToolsFramework/UI/Logging/LogTableModel.h>

#include <AzQtComponents/Components/Style.h>

namespace AzToolsFramework
{
    namespace Logging
    {
        QRect LogTableItemDelegate::itemViewItemRect(const AzQtComponents::Style* style, QStyle::SubElement element, const QStyleOptionViewItem* option, const QWidget* widget, const AzQtComponents::TableView::Config& config)
        {
            Q_UNUSED(config);
            if (!option->state.testFlag(QStyle::State_Selected))
            {
                return {};
            }

            switch (element)
            {
                case QStyle::SE_ItemViewItemText:
                {
                    if (option->index.column() != LogTableModel::ColumnMessage)
                    {
                        QRect styleRect = style->QProxyStyle::subElementRect(element, option, widget);
                        styleRect.setHeight(unexpandedRowHeight(option->index, widget));
                        return styleRect;
                    }

                    break;
                }

                case QStyle::SE_ItemViewItemDecoration:
                {
                    if (option->index.column() == LogTableModel::ColumnType)
                    {
                        QRect styleRect = style->QProxyStyle::subElementRect(element, option, widget);
                        QSize rectSize = styleRect.size();
                        QSize sh = sizeHint(*option, option->index);
                        styleRect.setTop(styleRect.top() - (sh.height() - unexpandedRowHeight(option->index, widget)) / 2);
                        styleRect.setSize(rectSize);
                        return styleRect;
                    }

                    break;
                }
            }

            return {};
        }

        void LogTableItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
        {
            AzQtComponents::TableViewItemDelegate::initStyleOption(option, index);

            // Only wrap text if this is the message column. By default, the TableViewItemDelegate
            // wraps the text for every column when selected
            if (!option->state.testFlag(QStyle::State_Selected)
                    || (index.column() != LogTableModel::ColumnMessage))
            {
                option->features &= ~(QStyleOptionViewItem::WrapText);
            }
        }

        QSize LogTableItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            // Assume that every row in a particular column will have the same size, except for the selected/expanded item.
            // Logs are often huge; figuring out the size of each item in a massive log takes forever, so we make
            // some assumptions and cache the size so it doesn't have to be recalculated for every item in the table.
            // The selected row is special; it expands. So check for that, and do so via checking with the base delegate class,
            // because we don't have direct access to the model here, and QStyle::State_Selected is not reliable in
            // sizeHint.

            if ((isSelected(index)) || (option.state.testFlag(QStyle::State_Selected)))
            {
                return AzQtComponents::TableViewItemDelegate::sizeHint(option, index);
            }

            auto item = m_cachedSizes.find(index.column());
            if (item != m_cachedSizes.end())
            {
                return item->second;
            }

            QSize returnValue = AzQtComponents::TableViewItemDelegate::sizeHint(option, index);

            m_cachedSizes.insert(AZStd::make_pair(index.column(), returnValue));

            return returnValue;
        }

        void LogTableItemDelegate::geometriesUpdating(AzQtComponents::TableView* /*tableView*/)
        {
            m_cachedSizes.clear();
        }

        void LogTableItemDelegate::clearCachedValues()
        {
            AzQtComponents::TableViewItemDelegate::clearCachedValues();

            m_cachedSizes.clear();
        }

    } // namespace Logging
} // namespace AzToolsFramework

#include "UI/Logging/moc_LogTableItemDelegate.cpp"
