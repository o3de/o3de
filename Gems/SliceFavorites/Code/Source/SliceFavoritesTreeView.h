/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SLICE_FAVORITES_TREE_VIEW_H
#define SLICE_FAVORITES_TREE_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QTreeView>
#endif

#pragma once

namespace SliceFavorites
{
    //! This class exists to override the startDrag call to allow us to sort and generate a drag image
    class SliceFavoritesTreeView
        : public QTreeView
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(SliceFavoritesTreeView, AZ::SystemAllocator, 0);

        SliceFavoritesTreeView(QWidget* pParent = NULL);
        virtual ~SliceFavoritesTreeView();

    protected:
        // Qt overrides
        void startDrag(Qt::DropActions supportedActions) override;

    private:
        QModelIndexList SortIndicesByHierarchicalLocation(const QModelIndexList& indexList) const;
        void ConstructHierarchyData(const QModelIndex& index, AZStd::list<int>& hierarchy) const;
        QImage createDragImage(const QModelIndexList& indexList) const;

    };
}

#endif
