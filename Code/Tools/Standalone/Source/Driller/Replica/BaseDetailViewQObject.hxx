/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_BASEDETAILVIEWQOBJECT_H
#define DRILLER_REPLICA_BASEDETAILVIEWQOBJECT_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QDialog>
#include <QtWidgets/qtreeview.h>
#endif

namespace Ui
{
    class BaseDetailView;
}

namespace Driller
{
    class ReplicaDataView;

    // This class is a work around because QT does not play nicely with templates.
    // So I'm going to do all of my Qt related signalling here and just pass it along
    // to a virtual function.
    class BaseDetailViewQObject : public QDialog
    {
        Q_OBJECT
    public:        
    
        AZ_CLASS_ALLOCATOR(BaseDetailViewQObject, AZ::SystemAllocator,0);
        
        BaseDetailViewQObject(QWidget* parent = nullptr);

        void SetupSignals(ReplicaDataView* dataView, Ui::BaseDetailView* detailView);
        void SetupTreeViewSignals(QTreeView* treeView);

    public slots:
        void DataRangeChanged();
        void HideAll();
        void ShowAll();

        void HideSelected();
        void ShowSelected();

        void CollapseAll();        
        void ExpandAll();

        void DoubleClicked(const QModelIndex& index);
        void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void UpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex);

        void DisplayModeChanged(int);
        void GraphDetailChanged(int);
        void BandwidthDisplayUsageTypeChanged(int);
        
        void InspectedSeries(size_t seriesId);
        void SelectedSeries(size_t seriesId, int position);

    protected:
        virtual void OnDataRangeChanged() = 0;

        virtual void SetAllEnabled(bool enabled) = 0;
        virtual void SetSelectedEnabled(bool enabled) = 0;

        virtual void OnCollapseAll() = 0;
        virtual void OnExpandAll() = 0;

        virtual void OnDoubleClicked(const QModelIndex& index) = 0;
        virtual void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) = 0;
        virtual void OnUpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex) = 0;

        virtual void OnDisplayModeChanged(int) = 0;
        virtual void OnGraphDetailChanged(int) = 0;
        virtual void OnBandwidthDisplayUsageTypeChanged(int) = 0;

        virtual void OnInspectedSeries(size_t seriesId) = 0;
        virtual void OnSelectedSeries(size_t seriesId, int position) = 0;
    };
}

#endif
