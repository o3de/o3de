/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_UI_UICORE_FILTEREDLISTVIEW_H
#define AZTOOLSFRAMEWORK_UI_UICORE_FILTEREDLISTVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QListView>
#include <qstringlistmodel.h>
#include <qstringlist.h>
#include <qsortfilterproxymodel.h>
#endif

namespace Ui
{
    class FilteredListView;
}

namespace Driller
{
    // This widget allows for easy access to a filtered list view that can also be rearranged by the user.
    // Useful for things like determining column order, or export fields.
    class FilteredListView 
        : public QWidget
    {
        Q_OBJECT

        // Apparently we can't just have a non-sorted model...
        class FilteredProxyModel : public QSortFilterProxyModel
        {
            void sort(int column, Qt::SortOrder order)
            {
                (void) column; (void)order;
            }
        };

    public:
        AZ_CLASS_ALLOCATOR(FilteredListView, AZ::SystemAllocator,0);

        explicit FilteredListView(QWidget* parent = nullptr);
        ~FilteredListView();

        void addItem(const char* item);
        void addItems(const QStringList& items);

        void removeItem(const char* item);
        void removeItems(const QStringList& items);

        void clearItems();

        void removeSelected();

        void setFilterString(const char* filterString);
        const char* getFilterString() const;

        const QStringList& getAllItems() const;
        void getSelectedItems(QStringList& selectedItems) const;

        void enableCustomOrdering(bool enabled);

    public slots:
        void filterEdited(const QString& eventFilter);
        void moveSelectionUp();
        void moveSelectionDown();

    private:

        void setButtonsEnabled(bool enabled);

        Ui::FilteredListView*  m_gui;
        bool                     m_enableCustomOrdering;
        FilteredProxyModel       m_filteredModel;
        QStringListModel         m_stringListModel;
        QStringList              m_stringList;
    };
}

#endif
