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

#ifndef SLICE_FAVORITES_WIDGET_H
#define SLICE_FAVORITES_WIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>

#include <QItemSelection>
#include <QScopedPointer>
#include <QWidget>
#endif

#pragma once

class QAction;

namespace Ui
{
    class SliceFavoritesWidgetUI;
}

namespace SliceFavorites
{
    class FavoriteDataModel;

    class SliceFavoritesWidget
        : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(SliceFavoritesWidget, AZ::SystemAllocator, 0)

        SliceFavoritesWidget(FavoriteDataModel* dataModel, QWidget* pParent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~SliceFavoritesWidget();

        private Q_SLOTS:
        void OnOpenTreeContextMenu(const QPoint& pos);


    private:
        QScopedPointer<Ui::SliceFavoritesWidgetUI> m_gui;
        FavoriteDataModel* m_dataModel;
        
        void RemoveSelection();
        void AddNewFolder(const QModelIndex& currentIndex);
        void UpdateWidget();

        bool CanAddNewFolder(const QItemSelection& selected);

        QAction* m_removeAction = nullptr;
    };
}

#endif
