/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "BaseDetailViewQObject.hxx"
#include <Source/Driller/Replica/moc_BaseDetailViewQObject.cpp>
#include <Source/Driller/Replica/ui_basedetailview.h>

#include "Source/Driller/AreaChart.hxx"
#include "Source/Driller/Replica/ReplicaDataView.hxx"

namespace Driller
{
    BaseDetailViewQObject::BaseDetailViewQObject(QWidget* parent)
        : QDialog(parent)
    {
    }

    void BaseDetailViewQObject::SetupSignals(ReplicaDataView* dataView, Ui::BaseDetailView* detailView)
    {
        QObject::connect(dataView,SIGNAL(DataRangeChanged()),this,SLOT(DataRangeChanged()));
        QObject::connect(detailView->treeView,SIGNAL(doubleClicked(const QModelIndex&)),this,SLOT(DoubleClicked(const QModelIndex&)));
        QObject::connect(detailView->bandwidthUsageDisplayType, SIGNAL(currentIndexChanged(int)), this, SLOT(BandwidthDisplayUsageTypeChanged(int)));
        QObject::connect(detailView->graphDetailType, SIGNAL(currentIndexChanged(int)), this, SLOT(GraphDetailChanged(int)));

        detailView->areaChart->EnableMouseInspection(true);

        QObject::connect(detailView->areaChart, SIGNAL(InspectedSeries(size_t)), this, SLOT(InspectedSeries(size_t)));
        QObject::connect(detailView->areaChart, SIGNAL(SelectedSeries(size_t, int)), this, SLOT(SelectedSeries(size_t, int)));
        
        QObject::connect(detailView->configToolbar,SIGNAL(hideAll()),this,SLOT(HideAll()));
        QObject::connect(detailView->configToolbar,SIGNAL(hideSelected()),this,SLOT(HideSelected()));
        QObject::connect(detailView->configToolbar,SIGNAL(showAll()),this,SLOT(ShowAll()));
        QObject::connect(detailView->configToolbar,SIGNAL(showSelected()),this,SLOT(ShowSelected()));
        QObject::connect(detailView->configToolbar,SIGNAL(collapseAll()),this,SLOT(CollapseAll()));
        QObject::connect(detailView->configToolbar,SIGNAL(expandAll()),this,SLOT(ExpandAll()));        
    }

    void BaseDetailViewQObject::SetupTreeViewSignals(QTreeView* treeView)
    {
        QObject::connect(treeView->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),this, SLOT(SelectionChanged(const QItemSelection&, const QItemSelection&)));
    }

    void BaseDetailViewQObject::DataRangeChanged()
    {
        OnDataRangeChanged();
    }

    void BaseDetailViewQObject::HideAll()
    {
        SetAllEnabled(false);
    }

    void BaseDetailViewQObject::ShowAll()
    {
        SetAllEnabled(true);
    }

    void BaseDetailViewQObject::HideSelected()
    {
        SetSelectedEnabled(false);
    }

    void BaseDetailViewQObject::ShowSelected()
    {
        SetSelectedEnabled(true);
    }

    void BaseDetailViewQObject::CollapseAll()
    {
        OnCollapseAll();
    }

    void BaseDetailViewQObject::ExpandAll()
    {
        OnExpandAll();
    }

    void BaseDetailViewQObject::DoubleClicked(const QModelIndex& index)
    {
        OnDoubleClicked(index);
    }

    void BaseDetailViewQObject::SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        OnSelectionChanged(selected,deselected);
    }

    void BaseDetailViewQObject::UpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex)
    {
        OnUpdateDisplay(startIndex,endIndex);
    }

    void BaseDetailViewQObject::DisplayModeChanged(int aggregationType)
    {
        OnDisplayModeChanged(aggregationType);
    }

    void BaseDetailViewQObject::GraphDetailChanged(int graphDetailType)
    {
        OnGraphDetailChanged(graphDetailType);
    }

    void BaseDetailViewQObject::BandwidthDisplayUsageTypeChanged(int bandwidthUsageType)
    {
        OnBandwidthDisplayUsageTypeChanged(bandwidthUsageType);
    }

    void BaseDetailViewQObject::InspectedSeries(size_t seriesId)
    {
        OnInspectedSeries(seriesId);
    }

    void BaseDetailViewQObject::SelectedSeries(size_t seriesId, int position)
    {
        OnSelectedSeries(seriesId, position);
    }
}
