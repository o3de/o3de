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

#ifndef DRILLER_REPLICA_BASEDETAILVIEW_H
#define DRILLER_REPLICA_BASEDETAILVIEW_H

#include <AzCore/base.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/conversions.h>

#include <QAbstractItemModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QItemSelectionModel>

#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include "Woodpecker/Driller/StripChart.hxx"
#include "Woodpecker/Driller/Replica/BaseDetailViewQObject.hxx"
#include "Woodpecker/Driller/Replica/ReplicaBandwidthChartData.h"
#include "Woodpecker/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Woodpecker/Driller/Replica/ReplicaDisplayTypes.h"
#include "Woodpecker/Driller/Replica/ReplicaDataView.hxx"
#include "Woodpecker/Driller/Replica/ReplicaTreeViewModel.hxx"
#include "Woodpecker/Driller/Replica/BaseDetailViewSavedState.h"

#include <Woodpecker/Driller/Replica/ui_basedetailview.h>

namespace Driller
{
    template<typename Key>
    class BaseDetailTreeViewModel;

    template<typename Key>
    class BaseDetailView
        : public BaseDetailViewQObject
    {
        typedef AZStd::unordered_set<Key> IdSet;

        friend class BaseDetailTreeViewModel<Key>;
        friend class ReplicaDataView;

    protected:

        enum class DisplayMode
        {
            Unknown = -2,
            Start,
            Active,
            Aggregate,
            End
        };

        enum class DetailMode
        {
            Unknown = -2,
            Start,
            Low,
            Medium,
            High,
            End
        };
        
    public:
        AZ_CLASS_ALLOCATOR(BaseDetailView, AZ::SystemAllocator, 0);

        BaseDetailView(ReplicaDataView* replicaDataView)
            : BaseDetailViewQObject(nullptr)
            , m_displayMode(DisplayMode::Unknown)
            , m_detailMode(DetailMode::Low)
            , m_bandwidthUsageDisplayType(ReplicaDisplayTypes::BUDT_COMBINED)
            , m_windowStateCRC(0)            
            , m_splitterStateCRC(0)
            , m_treeStateCRC(0)
            , m_replicaDataView(replicaDataView)
            , m_gui(nullptr)
        {
            setAttribute(Qt::WA_DeleteOnClose, true);
            setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

            m_gui = azcreate(Ui::BaseDetailView, ());
            m_gui->setupUi(this);

            for (int i = static_cast<int>(DetailMode::Start) + 1; i < static_cast<int>(DetailMode::End); ++i)
            {
                switch (static_cast<DetailMode>(i))
                {
                case DetailMode::High:
                    m_gui->graphDetailType->addItem("High");
                    break;
                case DetailMode::Medium:
                    m_gui->graphDetailType->addItem("Medium");
                    break;
                case DetailMode::Low:
                    m_gui->graphDetailType->addItem("Low");
                    break;
                default:
                    AZ_Error("Woodpecker", false, "Unknown GraphDetailMode.");
                    m_gui->graphDetailType->addItem("???");
                    break;
                }
            }

            m_gui->graphDetailType->setCurrentIndex(static_cast<int>(m_detailMode));

            for (int i = ReplicaDisplayTypes::BUDT_START + 1; i < ReplicaDisplayTypes::BUDT_END; ++i)
            {
                switch (i)
                {
                case ReplicaDisplayTypes::BUDT_COMBINED:
                    m_gui->bandwidthUsageDisplayType->addItem(ReplicaDisplayTypes::DisplayNames::BUDT_COMBINED_NAME);
                    break;
                case ReplicaDisplayTypes::BUDT_SENT:
                    m_gui->bandwidthUsageDisplayType->addItem(ReplicaDisplayTypes::DisplayNames::BUDT_SENT_NAME);
                    break;
                case ReplicaDisplayTypes::BUDT_RECEIVED:
                    m_gui->bandwidthUsageDisplayType->addItem(ReplicaDisplayTypes::DisplayNames::BUDT_RECEIVED_NAME);
                    break;
                default:
                    AZ_Error("Woodpecker", false, "Unknown Bandwidth Usage Display Type");
                    m_gui->bandwidthUsageDisplayType->addItem("???");
                    break;
                }
            }

            m_gui->bandwidthUsageDisplayType->setCurrentIndex(m_bandwidthUsageDisplayType);

            SetupSignals(m_replicaDataView,m_gui);
        }

        void LoadSavedState()
        {
            m_windowStateCRC = CreateWindowGeometryCRC();

            AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);

            if (windowState)
            {
                windowState->RestoreGeometry(this);
            }

            m_splitterStateCRC = CreateSplitterStateCRC();

            auto splitterState = AZ::UserSettings::Find<BaseDetailViewSplitterSavedState>(m_splitterStateCRC, AZ::UserSettings::CT_GLOBAL);

            if (splitterState)
            {
                QByteArray splitterData((const char*)splitterState->m_splitterStorage.data(), (int)splitterState->m_splitterStorage.size());
                m_gui->splitter->restoreState(splitterData);                
            }

            m_treeStateCRC = CreateTreeStateCRC();

            auto treeState = AZ::UserSettings::Find<BaseDetailViewTreeSavedState>(m_treeStateCRC, AZ::UserSettings::CT_GLOBAL);

            if (treeState)
            {
                QByteArray treeData((const char*)treeState->m_treeColumnStorage.data(), (int)treeState->m_treeColumnStorage.size());
                m_gui->treeView->header()->restoreState(treeData);
            }
        }

        ~BaseDetailView()
        {
            // Save out whatever data we want to save out.
            auto pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
            if (pState)
            {
                pState->CaptureGeometry(this);
            }

            auto splitterState = AZ::UserSettings::CreateFind<BaseDetailViewSplitterSavedState>(m_splitterStateCRC, AZ::UserSettings::CT_GLOBAL);
            if (splitterState)
            {
                QByteArray qba = m_gui->splitter->saveState();
                splitterState->m_splitterStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }

            auto treeState = AZ::UserSettings::CreateFind<BaseDetailViewTreeSavedState>(m_treeStateCRC, AZ::UserSettings::CT_GLOBAL);
            if (treeState)
            {
                if (m_gui->treeView && m_gui->treeView->header())
                {
                    QByteArray qba = m_gui->treeView->header()->saveState();
                    treeState->m_treeColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
                }
            }

            if (m_replicaDataView)
            {
                m_replicaDataView->SignalDialogClosed(this);
            }

            azdestroy(m_gui);
        }

        void RedrawGraph()
        {
            AZ_PROFILE_TIMER("Woodpecker", __FUNCTION__);
            switch (m_displayMode)
            {
            case DisplayMode::Active:
                DrawActiveGraph();
                break;
            case DisplayMode::Aggregate:
                DrawAggregateGraph();
                break;
            default:
                AZ_Error("BaseDetailView", false, "Trying to display unknown graph configuration.");
            }
        }

        void SetupTreeView()
        {
            m_gui->treeView->reset();

            m_gui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
            m_gui->treeView->setExpandsOnDoubleClick(false);


            m_gui->treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
            m_gui->treeView->header()->setStretchLastSection(false);

            OnSetupTreeView();
            SetupTreeViewSignals(m_gui->treeView);
        }

    public:
        virtual const typename ReplicaBandwidthChartData<Key>::FrameMap & GetFrameData() const = 0;
        virtual BaseDetailDisplayHelper* FindDetailDisplay(const Key& id) = 0;
        virtual const BaseDetailDisplayHelper* FindDetailDisplay(const Key& id) const = 0;

        virtual BaseDetailDisplayHelper* FindAggregateDisplay() { return nullptr; }
        virtual Key FindAggregateID() const { return Key(); }

    protected:
        void OnDataRangeChanged() override
        {
            InitializeDisplayData();
            RedrawGraph();
            ShowTreeFrame(m_replicaDataView->GetCurrentFrame());
        }

        void SetAllEnabled(bool enabled) override
        {
            BaseDetailDisplayHelper* aggregateDisplay = FindAggregateDisplay();

            if (aggregateDisplay)
            {
                aggregateDisplay->m_graphEnabled = enabled;

                BaseDisplayHelper* displayHelper = aggregateDisplay->GetDataSetDisplayHelper();
                displayHelper->m_graphEnabled = false;

                const AZStd::vector< BaseDisplayHelper* >& dataSets = displayHelper->GetChildren();

                for (BaseDisplayHelper* dataSet : dataSets)
                {
                    dataSet->m_graphEnabled = enabled;
                }

                displayHelper = aggregateDisplay->GetRPCDisplayHelper();
                displayHelper->m_graphEnabled = false;

                const AZStd::vector< BaseDisplayHelper* >& rpcs = displayHelper->GetChildren();

                for (BaseDisplayHelper* rpc : rpcs)
                {
                    rpc->m_graphEnabled = enabled;
                }
            }

            for (Key& currentId : m_activeIds)
            {
                BaseDetailDisplayHelper* detailHelper = FindDetailDisplay(currentId);
                detailHelper->m_graphEnabled = enabled;

                BaseDisplayHelper* displayHelper = detailHelper->GetDataSetDisplayHelper();
                displayHelper->m_graphEnabled = enabled;

                const AZStd::vector< BaseDisplayHelper* >& dataSets = displayHelper->GetChildren();

                for (BaseDisplayHelper* dataSet : dataSets)
                {
                    dataSet->m_graphEnabled = enabled;
                }

                displayHelper = detailHelper->GetRPCDisplayHelper();
                displayHelper->m_graphEnabled = enabled;

                const AZStd::vector< BaseDisplayHelper* >& rpcs = displayHelper->GetChildren();

                for (BaseDisplayHelper* rpc : rpcs)
                {
                    rpc->m_graphEnabled = enabled;
                }
            }

            LayoutChanged();
            RedrawGraph();
        }

        void SetSelectedEnabled(bool enabled) override
        {
            QModelIndexList selection = m_gui->treeView->selectionModel()->selectedIndexes();

            for (QModelIndex& index : selection)
            {
                static_cast<BaseDisplayHelper*>(index.internalPointer())->m_graphEnabled = enabled;
            }

            LayoutChanged();
            RedrawGraph();
        }

        void OnCollapseAll() override
        {
            m_gui->treeView->collapseAll();
        }

        void OnExpandAll() override
        {
            m_gui->treeView->expandAll();
        }

        void OnDoubleClicked(const QModelIndex& clickedIndex) override
        {
            if (!clickedIndex.isValid())
            {
                return;
            }

            BaseDisplayHelper* displayHelper = static_cast<BaseDisplayHelper*>(clickedIndex.internalPointer());

            displayHelper->m_graphEnabled = !displayHelper->m_graphEnabled;

            LayoutChanged();
            RedrawGraph();
        }        

        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override
        {
            for (const QModelIndex& selectedIndex : selected.indexes())
            {
                BaseDisplayHelper* displayHelper = static_cast<BaseDisplayHelper*>(selectedIndex.internalPointer());

                displayHelper->m_selected = true;
                displayHelper->m_areaGraphPlotHelper.SetHighlighted(displayHelper->m_selected);                
            }

            for (const QModelIndex& deselectedIndex : deselected.indexes())
            {
                BaseDisplayHelper* displayHelper = static_cast<BaseDisplayHelper*>(deselectedIndex.internalPointer());

                displayHelper->m_selected = false;
                displayHelper->m_areaGraphPlotHelper.SetHighlighted(displayHelper->m_selected);                
            }
        }

        void OnUpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex) override
        {
            (void)startIndex;
            (void)endIndex;

            RedrawGraph();
        }

        void OnDisplayModeChanged(int displayMode) override
        {
            if (displayMode > static_cast<int>(DisplayMode::Start) && displayMode < static_cast<int>(DisplayMode::End))
            {
                if (m_displayMode != static_cast<DisplayMode>(displayMode))
                {
                    m_displayMode = static_cast<DisplayMode>(displayMode);

                    InitializeDisplayData();
                    SetupTreeView();
                    RedrawGraph();
                }
            }
        }

        void OnGraphDetailChanged(int graphDetail) override
        {
            if (graphDetail > static_cast<int>(DetailMode::Start) && graphDetail < static_cast<int>(DetailMode::End))
            {
                if (m_detailMode != static_cast<DetailMode>(graphDetail))
                {
                    m_detailMode = static_cast<DetailMode>(graphDetail);
                    RedrawGraph();
                }
            }
        }

        void OnBandwidthDisplayUsageTypeChanged(int bandwidthUsageType)
        {
            if (bandwidthUsageType > ReplicaDisplayTypes::BUDT_START && bandwidthUsageType < ReplicaDisplayTypes::BUDT_END)
            {
                if (m_bandwidthUsageDisplayType != static_cast<ReplicaDisplayTypes::BandwidthUsageDisplayType>(bandwidthUsageType))
                {
                    m_bandwidthUsageDisplayType = static_cast<ReplicaDisplayTypes::BandwidthUsageDisplayType>(bandwidthUsageType);
                    RedrawGraph();
                }
            }
        }

        void OnInspectedSeries(size_t seriesId)
        {
            (void)seriesId;

            // Nothing to see here
        }

        void OnSelectedSeries(size_t seriesId, int position)
        {
            (void)seriesId;

            EBUS_EVENT_ID(m_replicaDataView->GetCaptureWindowIdentity(), DrillerCaptureWindowRequestBus, ScrubToFrameRequest, position);
        }

        bool IsInDisplayMode(DisplayMode displayMode)
        {
            return m_displayMode == displayMode;
        }

    protected:

        void SignalDataViewDestroyed(ReplicaDataView* dataView)
        {
            if (dataView == m_replicaDataView)
            {
                m_replicaDataView = nullptr;
            }

            close();
        }

        virtual void InitializeDisplayData() = 0;
        virtual void LayoutChanged() = 0;
        virtual void OnSetupTreeView() = 0;
        virtual void ShowTreeFrame(FrameNumberType frameId) = 0;

        virtual AZ::u32 CreateWindowGeometryCRC() = 0;
        virtual AZ::u32 CreateSplitterStateCRC() = 0;
        virtual AZ::u32 CreateTreeStateCRC() = 0;

        DisplayMode                                m_displayMode;        
        DetailMode                                 m_detailMode;

        ReplicaDisplayTypes::BandwidthUsageDisplayType m_bandwidthUsageDisplayType;

        AZ::u32                m_windowStateCRC;
        AZ::u32                m_splitterStateCRC;
        AZ::u32                m_treeStateCRC;

        ReplicaDataView*       m_replicaDataView;        
        IdSet                  m_activeIds;
        IdSet                  m_activeInspectedIds;
        Ui::BaseDetailView*    m_gui;

    private:
        void DrawActiveGraph();
        void DrawHighDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId);
        void DrawMediumDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId);
        void DrawLowDetailActiveGraph(BaseDetailDisplayHelper* detailDisplayHelper, const BandwidthUsageContainer* usageContainer, FrameNumberType frameId);

        void DrawAggregateGraph();        

        void PlotBatchedGraphData(AreaGraphPlotHelper& areaPlotHelper, FrameNumberType frameId, const BandwidthUsageAggregator& usageAggregator)
        {
            switch (m_bandwidthUsageDisplayType)
            {
            case ReplicaDisplayTypes::BUDT_COMBINED:
                areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(usageAggregator.m_bytesSent + usageAggregator.m_bytesReceived));
                break;
            case ReplicaDisplayTypes::BUDT_SENT:
                areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(usageAggregator.m_bytesSent));
                break;
            case ReplicaDisplayTypes::BUDT_RECEIVED:
                areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(usageAggregator.m_bytesReceived));
                break;
            default:
                AZ_Error("BaseDetailView", false, "Unknown bandwidth usage display type.");
            }
        }

        void ConfigureBaseDetailDisplayHelper(BaseDetailDisplayHelper* detailDisplayHelper);
        
        void ConfigureGraphAxis()
        {
            const QColor markerColor(Qt::red);
            m_gui->areaChart->ResetChart();
            m_gui->areaChart->ConfigureVerticalAxis("Bandwidth Usage", m_replicaDataView->GetAverageFrameBandwidthBudget());
            m_gui->areaChart->ConfigureHorizontalAxis("Frame", static_cast<int>(m_replicaDataView->GetAxisStartFrame()), static_cast<int>(m_replicaDataView->GetAxisEndFrame()));            
            m_gui->areaChart->AddMarker(Charts::AxisType::Horizontal, static_cast<int>(m_replicaDataView->GetCurrentFrame()), markerColor);
        }
    };

    template<class Key>
    class BaseDetailTreeViewModel
        : public ReplicaTreeViewModel
    {
        typedef ReplicaBandwidthChartData<Key> BandwidthChartData;
    public:
        AZ_CLASS_ALLOCATOR(BaseDetailTreeViewModel, AZ::SystemAllocator, 0);
        BaseDetailTreeViewModel(BaseDetailView<Key>* detailView)
            : ReplicaTreeViewModel(nullptr)
            , m_baseDetailView(detailView)
        {
        }

        void RefreshView(FrameNumberType frameId)
        {
            AZ_PROFILE_TIMER("Woodpecker", __FUNCTION__);
            AZStd::unordered_set< Key > discoveredSet;

            m_tableViewOrdering.clear();

            if (m_baseDetailView->IsInDisplayMode(BaseDetailView<Key>::DisplayMode::Active))
            {
                if (m_baseDetailView->m_replicaDataView->HideInactiveInspectedElements())
                {
                    m_tableViewOrdering.insert(m_tableViewOrdering.begin(), m_baseDetailView->m_activeInspectedIds.begin(), m_baseDetailView->m_activeInspectedIds.end());
                }
                else
                {
                    m_tableViewOrdering.insert(m_tableViewOrdering.begin(), m_baseDetailView->m_activeIds.begin(), m_baseDetailView->m_activeIds.end());
                }

            }

            const typename BandwidthChartData::FrameMap& frameMap = m_baseDetailView->GetFrameData();
            auto frameIter = frameMap.find(frameId);

            BaseDetailDisplayHelper* aggregateDisplayHelper = m_baseDetailView->FindAggregateDisplay();

            if (aggregateDisplayHelper && m_baseDetailView->IsInDisplayMode(BaseDetailView<Key>::DisplayMode::Aggregate))
            {
                aggregateDisplayHelper->ResetBandwidthUsage();

                if (m_baseDetailView->m_replicaDataView->HideInactiveInspectedElements())
                {
                    if (frameIter != frameMap.end())
                    {
                        m_tableViewOrdering.push_back(m_baseDetailView->FindAggregateID());    
                    }
                }
                else
                {
                    m_tableViewOrdering.push_back(m_baseDetailView->FindAggregateID());
                }
            }
            else
            {
                aggregateDisplayHelper = nullptr;
            }

            if (frameIter != frameMap.end())
            {
                const typename BandwidthChartData::BandwidthUsageMap* usageMap = frameIter->second;

                for (typename BandwidthChartData::BandwidthUsageMap::const_iterator usageIter = usageMap->begin();
                     usageIter != usageMap->end();
                     ++usageIter)
                {
                    const BandwidthUsageContainer* usageContainer = usageIter->second;

                    const Key& idKey = usageIter->first;

                    BaseDetailDisplayHelper* detailHelper = m_baseDetailView->FindDetailDisplay(idKey);

                    if (detailHelper)
                    {
                        auto insert = discoveredSet.insert(idKey);

                        if (insert.second)
                        {
                            detailHelper->ResetBandwidthUsage();
                        }

                        DataSetDisplayFilter* dataSetDisplayFilter = detailHelper->GetDataSetDisplayHelper();

                        if (dataSetDisplayFilter)
                        {
                            const BandwidthUsageContainer::UsageAggregationMap& dataSetUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::DATA_SET);

                            for (const auto& usagePair : dataSetUsage)
                            {
                                const BandwidthUsage& currentUsage = usagePair.second;

                                detailHelper->AddDataSetUsage(currentUsage);

                                if (aggregateDisplayHelper)
                                {
                                    aggregateDisplayHelper->AddDataSetUsage(currentUsage);
                                }
                            }
                        }

                        RPCDisplayFilter* rpcDisplayFilter = detailHelper->GetRPCDisplayHelper();

                        if (rpcDisplayFilter)
                        {
                            const BandwidthUsageContainer::UsageAggregationMap& rpcUsage = usageContainer->GetDataTypeUsageAggregation(BandwidthUsage::DataType::REMOTE_PROCEDURE_CALL);

                            for (const auto& usagePair : rpcUsage)
                            {
                                const BandwidthUsage& currentUsage = usagePair.second;

                                detailHelper->AddRPCUsage(currentUsage);

                                if (aggregateDisplayHelper)
                                {
                                    aggregateDisplayHelper->AddRPCUsage(currentUsage);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (aggregateDisplayHelper)
                {
                    aggregateDisplayHelper->ResetBandwidthUsage();
                }

                for (Key& currentId : m_tableViewOrdering)
                {
                    BaseDetailDisplayHelper* detailDisplayHelper = m_baseDetailView->FindDetailDisplay(currentId);

                    if (detailDisplayHelper)
                    {
                        detailDisplayHelper->ResetBandwidthUsage();
                    }
                }
            }

            AZStd::sort(m_tableViewOrdering.begin(), m_tableViewOrdering.end(), AZStd::less<Key>());

            layoutChanged();
        }

    protected:

        int GetRootRowCount() const override
        {
            return static_cast<int>(m_tableViewOrdering.size());
        }

        const BaseDisplayHelper* FindDisplayHelperAtRoot(int row) const override
        {
            if (row < 0 || row >= m_tableViewOrdering.size())
            {
                return nullptr;
            }

            return m_baseDetailView->FindDetailDisplay(m_tableViewOrdering[row]);
        }

        BaseDetailView<Key>* m_baseDetailView;
        AZStd::vector< Key > m_tableViewOrdering;
    };
}

#include "BaseDetailView.inl"
#endif