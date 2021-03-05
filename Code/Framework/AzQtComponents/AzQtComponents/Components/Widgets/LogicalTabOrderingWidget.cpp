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

#include <AzQtComponents/Components/Widgets/LogicalTabOrderingWidget.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QVector>
#include <QScrollArea>

namespace AzQtComponents
{
    namespace LogicalTabOrderingInternal
    {
        void findTabKeyOrdering(QVector<QWidget*>& items, QLayout* layout);
        void findTabKeyOrdering(QVector<QWidget*>& items, QWidget* widget);
        void findTabKeyOrdering(QVector<QWidget*>& items, QLayoutItem* item);

        bool focusNextPrevChild(bool next, QMap<QObject*, TabKeyEntry>& entries, QWidget* first, QWidget* last)
        {
            if (entries.empty())
            {
                return false;
            }

            QWidget* currentFocus = QApplication::focusWidget();
            QWidget* newFocus = nullptr;
            auto entry = entries.find(currentFocus);
            if (entry != entries.end())
            {
                if (next)
                {
                    newFocus = entry.value().next;
                }
                else
                {
                    newFocus = entry.value().prev;
                }
            }
            else
            {
                if (next)
                {
                    newFocus = first;
                }
                else
                {
                    newFocus = last;
                }
            }

            if (newFocus)
            {
                Qt::FocusReason reason = next ? Qt::TabFocusReason : Qt::BacktabFocusReason;
                newFocus->setFocus(reason);
                return true;
            }

            return false;
        }

        void findTabKeyOrdering(QVector<QWidget*>& items, QWidget* widget)
        {
            if (!widget)
            {
                return;
            }

            if (widget->isWindow() || !widget->isVisible() || !widget->isEnabled())
            {
                return;
            }

            QWidget* proxy = widget->focusProxy();
            if (proxy)
            {
                items.push_back(proxy);
            }
            else if ((widget->focusPolicy() & Qt::TabFocus) != 0)
            {
                items.push_back(widget);
            }

            QScrollArea* scrollArea = qobject_cast<QScrollArea*>(widget);
            if (scrollArea)
            {
                QWidget* scrollWidget = scrollArea->widget();
                findTabKeyOrdering(items, scrollWidget);
            }

            QLayout* layout = widget->layout();
            findTabKeyOrdering(items, layout);
        }

        void createTabKeyMapping(const QVector<QWidget*>& items, QMap<QObject*, TabKeyEntry>& entries)
        {
            if (items.empty())
            {
                return;
            }

            QWidget* lastFocus = nullptr;
            for (QWidget* item : items)
            {
                TabKeyEntry entry = { nullptr, lastFocus };
                entries.insert(item, entry);

                if (lastFocus)
                {
                    entries[lastFocus].next = item;
                }

                lastFocus = item;
            }

            QWidget* firstFocus = items[0];
            entries[lastFocus].next = firstFocus;
            entries[firstFocus].prev = lastFocus;
        }


        void findTabKeyOrdering(QVector<QWidget*>& items, QLayoutItem* item)
        {
            if (item)
            {
                QWidget* widget = item->widget();
                findTabKeyOrdering(items, widget);

                QLayout* subLayout = item->layout();
                findTabKeyOrdering(items, subLayout);
            }
        }

        void findTabKeyOrdering(QVector<QWidget*>& items, QLayout* layout)
        {
            if (!layout)
            {
                return;
            }

            if (QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>(layout))
            {
                for (int i = 0; i < boxLayout->count(); i++)
                {
                    QLayoutItem* item = boxLayout->itemAt(i);
                    findTabKeyOrdering(items, item);
                }
            }

            if (QGridLayout* gridLayout = qobject_cast<QGridLayout*>(layout))
            {
                for (int x = 0; x < gridLayout->columnCount(); x++)
                {
                    for (int y = 0; y < gridLayout->rowCount(); y++)
                    {
                        QLayoutItem* item = gridLayout->itemAtPosition(y, x);
                        findTabKeyOrdering(items, item);
                    }
                }
            }
        }

    } // namespace LogicalTabOrderingInternal
} // namespace AzQtComponents