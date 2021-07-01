/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROFILER_REPLICA_OVERALLDETAILVIEW_H
#define PROFILER_REPLICA_OVERALLDETAILVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/string.h>

#include <QAbstractTableModel>
#include <QMainWindow>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"

#include "Source/Driller/DrillerDataTypes.h"
#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Source/Driller/Replica/ReplicaTreeViewModel.hxx"

#include "ReplicaBandwidthChartData.h"
#endif

namespace Ui
{
    class OverallReplicaDetailView;
}

namespace AZ { class ReflectContext; }

namespace Driller
{
    class OverallReplicaDetailView;
    class ReplicaDataView;

    class ReplicaDataAggregator;
    
    class ReplicaChunkDataEvent;
    class ReplicaChunkReceivedDataEvent;
    class ReplicaChunkSentDataEvent;

    class AbstractOverallReplicaDetailView
        : public QDialog
    {
        template<typename Key>
        friend class BaseOverallTreeViewModel;

        friend class OverallReplicaTreeViewModel;
        friend class OverallReplicaChunkTypeTreeViewModel;

        Q_OBJECT
    public:
        virtual int GetFrameRange() const = 0;
        virtual int GetFPS() const = 0;

    protected:
        virtual OverallReplicaDetailDisplayHelper* FindReplicaDisplayHelper(AZ::u64 replicaId) = 0;
        virtual ReplicaChunkDetailDisplayHelper* FindReplicaChunkTypeDisplayHelper(const AZStd::string& chunkTypeName) = 0;

        BandwidthUsageAggregator m_totalUsageAggregator;
    };

    template<typename Key>
    class BaseOverallTreeViewModel : public ReplicaTreeViewModel
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseOverallTreeViewModel, AZ::SystemAllocator,0);
        BaseOverallTreeViewModel(AbstractOverallReplicaDetailView* overallDetailView)
            : m_overallReplicaDetailView(overallDetailView)
        {
        }

        AZStd::vector< Key > m_tableViewOrdering;

    protected:

        int GetRootRowCount() const override
        {
            return static_cast<int>(m_tableViewOrdering.size());
        }

        QVariant displayNameData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                QString displayName = baseDisplay->GetDisplayName();

                if (displayName.isEmpty())
                {
                    return QString("<unknown>");
                }
                else
                {
                    return displayName;
                }
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return QVariant(Qt::AlignVCenter);
            }

            return QVariant();
        }

        QVariant totalSentData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesSent);
            }
            else if (role == Qt::UserRole)
            {
                return baseDisplay->m_bandwidthUsageAggregator.m_bytesSent;
            }

            return QVariant();
        }

        QVariant totalReceivedData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived);
            }
            else if (role == Qt::UserRole)
            {
                return baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived;
            }

            return QVariant();
        }

        QVariant avgSentPerFrameData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesSent/m_overallReplicaDetailView->GetFrameRange());
            }
            else if (role == Qt::UserRole)
            {
                return baseDisplay->m_bandwidthUsageAggregator.m_bytesSent/m_overallReplicaDetailView->GetFrameRange();
            }

            return QVariant();
        }

        QVariant avgReceivedPerFrameData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number(baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived/m_overallReplicaDetailView->GetFrameRange());
            }
            else if (role == Qt::UserRole)
            {
                return baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived/m_overallReplicaDetailView->GetFrameRange();
            }

            return QVariant();
        }

        QVariant avgSentPerSecondData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number((baseDisplay->m_bandwidthUsageAggregator.m_bytesSent/m_overallReplicaDetailView->GetFrameRange()) * m_overallReplicaDetailView->GetFPS());
            }
            else if (role == Qt::UserRole)
            {
                return (baseDisplay->m_bandwidthUsageAggregator.m_bytesSent/m_overallReplicaDetailView->GetFrameRange()) * m_overallReplicaDetailView->GetFPS();
            }

            return QVariant();
        }

        QVariant avgReceivedPerSecondData(const BaseDisplayHelper* baseDisplay, int role) const
        {
            if (role == Qt::DisplayRole)
            {
                return QString::number((baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived/m_overallReplicaDetailView->GetFrameRange()) * m_overallReplicaDetailView->GetFPS());
            }
            else if (role == Qt::UserRole)
            {
                return (baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived/m_overallReplicaDetailView->GetFrameRange()) * m_overallReplicaDetailView->GetFPS();
            }

            return QVariant();
        }

        QVariant percentOfSentData(const BaseDisplayHelper* baseDisplay, int role, bool isRelative) const
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                size_t denominator = m_overallReplicaDetailView->m_totalUsageAggregator.m_bytesSent;

                if (isRelative)
                {
                    const BaseDisplayHelper* parentHelper = baseDisplay->GetParent();                    
                    if (parentHelper)
                    {
                        denominator = parentHelper->m_bandwidthUsageAggregator.m_bytesSent;
                    }
                }

                if (denominator == 0)
                {
                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(0,'f',3);
                    }
                    else
                    {
                        return QVariant(0.0f);
                    }
                }
                else
                {
                    float value = static_cast<float>(baseDisplay->m_bandwidthUsageAggregator.m_bytesSent)/static_cast<float>(denominator)*100.0f;

                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(value,'f',3);
                    }
                    else
                    {
                        return QVariant(value);
                    }
                }
            }

            return QVariant();
        }

        QVariant percentOfReceivedData(const BaseDisplayHelper* baseDisplay, int role, bool isRelative) const
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                size_t denominator = m_overallReplicaDetailView->m_totalUsageAggregator.m_bytesReceived;

                if (isRelative)
                {
                    const BaseDisplayHelper* parentHelper = baseDisplay->GetParent();                    
                    if (parentHelper)
                    {
                        denominator = parentHelper->m_bandwidthUsageAggregator.m_bytesReceived;
                    }
                }

                if (denominator == 0)
                {
                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(0,'f',3);
                    }
                    else
                    {
                        return QVariant(0.0f);
                    }
                }
                else
                {
                    float value = static_cast<float>(baseDisplay->m_bandwidthUsageAggregator.m_bytesReceived)/static_cast<float>(denominator)*100.0f;

                    if (role == Qt::DisplayRole)
                    {
                        return QString::number(value,'f',3);
                    }
                    else
                    {
                        return QVariant(value);
                    }
                }
            }

            return QVariant();
        }
        
        AbstractOverallReplicaDetailView* m_overallReplicaDetailView;
    };

    class OverallReplicaTreeViewModel : public BaseOverallTreeViewModel<AZ::u64>
    {
    public:
        enum ColumnDescriptor
        {
            // Forcing the index to start at 0
            CD_INDEX_FORCE = -1,
            
            // Ordering of this enum determines the display order in the tree
            CD_DISPLAY_NAME,
            CD_REPLICA_ID,
            
            CD_TOTAL_SENT,
            CD_AVG_SENT_FRAME,
            CD_AVG_SENT_SECOND,            
            CD_PARENT_PERCENT_SENT,
            CD_TOTAL_PERCENT_SENT,
            
            CD_TOTAL_RECEIVED,            
            CD_AVG_RECEIVED_FRAME,
            CD_AVG_RECEIVED_SECOND,            
            CD_PARENT_PERCENT_RECEIVED,
            CD_TOTAL_PERCENT_RECEIVED,
            
            // Used for sizing of the TableView. Anything after this won't be displayed.
            CD_COUNT
        };
        
        AZ_CLASS_ALLOCATOR(OverallReplicaTreeViewModel, AZ::SystemAllocator,0);
        OverallReplicaTreeViewModel(AbstractOverallReplicaDetailView* overallDetailView);
        
        int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    protected:
        const BaseDisplayHelper* FindDisplayHelperAtRoot(int row) const;
    };
    
    class OverallReplicaChunkTypeTreeViewModel : public BaseOverallTreeViewModel<AZStd::string>
    {
    public:
        enum ColumnDescriptor
        {
            // Forcing the index to start at 0
            CD_INDEX_FORCE = -1,
            
            // Ordering of this enum determines the display order in the tree
            CD_DISPLAY_NAME,            
            
            CD_TOTAL_SENT,
            CD_AVG_SENT_FRAME,
            CD_AVG_SENT_SECOND,
            CD_PARENT_PERCENT_SENT,
            CD_TOTAL_PERCENT_SENT,
            
            CD_TOTAL_RECEIVED,
            CD_AVG_RECEIVED_FRAME,            
            CD_AVG_RECEIVED_SECOND,            
            CD_PARENT_PERCENT_RECEIVED,
            CD_TOTAL_PERCENT_RECEIVED,
            
            // Used for sizing of the TableView. Anything after this won't be displayed.
            CD_COUNT
        };
        
        AZ_CLASS_ALLOCATOR(OverallReplicaChunkTypeTreeViewModel, AZ::SystemAllocator,0);
        OverallReplicaChunkTypeTreeViewModel(AbstractOverallReplicaDetailView* overallDetailView);
        
        int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    protected:
        const BaseDisplayHelper* FindDisplayHelperAtRoot(int row) const;
    };

    class OverallReplicaDetailView
        : public AbstractOverallReplicaDetailView
    {
    private:

        Q_OBJECT

        template<typename Key>
        friend class BaseOverallTreeViewModel;

        friend class OverallReplicaTreeViewModel;
        friend class OverallReplicaChunkTypeTreeViewModel;

        typedef AZStd::unordered_map<AZ::u64, OverallReplicaDetailDisplayHelper*> ReplicaDisplayHelperMap;
        typedef AZStd::unordered_map<AZStd::string, ReplicaChunkDetailDisplayHelper*> ReplicaChunkTypeDisplayHelperMap;
        
        static const char* WINDOW_STATE_FORMAT;
        static const char* REPLICA_TREE_STATE_FORMAT;
        static const char* REPLICA_CHUNK_TREE_STATE_FORMAT;
        
    public:    
        AZ_CLASS_ALLOCATOR(OverallReplicaDetailView, AZ::SystemAllocator, 0);
        
        OverallReplicaDetailView(ReplicaDataView* dataView, const ReplicaDataAggregator& dataAggregator);
        ~OverallReplicaDetailView();

        int GetFrameRange() const override;
        int GetFPS() const override;

        void SignalDataViewDestroyed(ReplicaDataView* replicaDataView);
        
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);
        void ApplyPersistentState();
        
        static void Reflect(AZ::ReflectContext* context);
        
    public slots:
    
        void OnFPSChanged(int);

        void QueueUpdate(int);
        void OnDataRangeChanged();        
        
    private:

        OverallReplicaDetailDisplayHelper* CreateReplicaDisplayHelper(const char* replicaName, AZ::u64 replicaId);
        OverallReplicaDetailDisplayHelper* FindReplicaDisplayHelper(AZ::u64 replicaId);

        ReplicaChunkDetailDisplayHelper* CreateReplicaChunkTypeDisplayHelper(const AZStd::string& chunkTypeName, AZ::u32 chunkIndex);
        ReplicaChunkDetailDisplayHelper* FindReplicaChunkTypeDisplayHelper(const AZStd::string& chunkTypeName);
    
        void SaveOnExit();
        void UpdateFrameBoundaries();
    
        void ParseData();
        void ProcessForReplica(ReplicaChunkEvent* chunkEvent);
        void ProcessForReplicaChunk(ReplicaChunkEvent* chunkEvent);
        void ProcessForBaseDetailDisplayHelper(ReplicaChunkEvent* chunkEvent, BaseDetailDisplayHelper* baseDetailDisplayHelper);

        void ClearData();
        
        void UpdateDisplay();
        
        void SetupTreeView();
        void SetupReplicaTreeView();
        void SetupReplicaChunkTypeTreeView();
    
        // Window Telemetry
        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;        

        ReplicaDataView* m_replicaDataView;

        // Window Saved State
        AZ::Crc32 m_windowStateCRC;
        AZ::Crc32 m_replicaTreeStateCRC;
        AZ::Crc32 m_replicaChunkTreeStateCRC;
        
        // General Data Source
        const ReplicaDataAggregator& m_dataAggregator;

        // Cached data
        int m_frameRange;

        // UX niceties
        QTimer m_changeTimer;
        
        // Display features for the Replica usage table
        OverallReplicaTreeViewModel m_overallReplicaModel;
        QSortFilterProxyModel m_replicaFilterProxyModel;        
        ReplicaDisplayHelperMap m_replicaDisplayHelpers;
        
        // Display features for the ReplicaChunkType usage table
        OverallReplicaChunkTypeTreeViewModel m_overallChunkTypeModel;
        QSortFilterProxyModel m_replicaChunkTypeFilterProxyModel;
        ReplicaChunkTypeDisplayHelperMap m_replicaChunkTypeDisplayHelpers;

        Ui::OverallReplicaDetailView* m_gui;
    };
}

#endif
