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
