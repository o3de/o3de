/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef TRACEDRILLERDIALOG_H
#define TRACEDRILLERDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QDialog>
#include <QtGui/QIcon>
#include <QtCore/QAbstractItemModel>

#include <AzToolsFramework/UI/Logging/LogControl.h>

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"
#endif

namespace Ui {
        class TraceDrillerDialog;
} // namespace Ui

namespace AZ { class ReflectContext; }

namespace Driller
{
    class TraceFilterModel;
    class TraceMessageDataAggregator;
    class TraceDrillerLogModel;
    class TraceDrillerDialogSavedState;

    class TraceDrillerDialog 
        : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TraceDrillerDialog, AZ::SystemAllocator, 0);

        TraceDrillerDialog(TraceMessageDataAggregator* data, int profilerIndex, QWidget *pParent = NULL);
        virtual ~TraceDrillerDialog();

        // NB: These three methods mimic the workspace bus.
        // Because the ProfilerDataAggregator can't know to open these DataView windows
        // until after the EBUS message has gone out, the owning aggregator must
        // first create these windows and then pass along the provider manually
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

        void SaveOnExit();
        virtual void closeEvent(QCloseEvent *evt);
        virtual void hideEvent(QHideEvent *evt);

        void ApplyPersistentState();

        AZ::u32 m_windowStateCRC;
        AZ::u32 m_filterStateCRC;
        int m_viewIndex;

        // persistent state is used as if they were internal variables,
        // though they reside in a storage class
        // this lasts for the entire lifetime of this object
        AZStd::intrusive_ptr<TraceDrillerDialogSavedState> m_persistentState;

    public slots:
        void OnDataDestroyed();
        void onTextChangeWindowFilter(const QString &);
        void onTextChangeMessageFilter(const QString &);

        void UpdateSummary();

    protected:
        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;

        Ui::TraceDrillerDialog*    m_uiLoaded;
        TraceFilterModel* m_ptrFilter;
        TraceDrillerLogModel* m_ptrOriginalModel;

    public:
        static void Reflect(AZ::ReflectContext* context);
    };

    class TraceDrillerLogTab : public AzToolsFramework::LogPanel::BaseLogView
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(TraceDrillerLogTab, AZ::SystemAllocator, 0);

        TraceDrillerLogTab(QWidget *pParent = NULL);
        virtual ~TraceDrillerLogTab();

        // return -1 for any of these to indicate that your data has no such column.
        // make sure your model has the same semantics!
        virtual int GetIconColumn() { return 0; }
        virtual int GetWindowColumn() { return 1; }
        virtual int GetMessageColumn() { return 2; }  // you may not return -1 for this one.
        virtual int GetTimeColumn() { return -1; }

       using AzToolsFramework::LogPanel::BaseLogView::rowsInserted;

    public slots:
        void  rowsAboutToBeInserted();
        void  rowsInserted ();
    
    protected:
        
        bool m_isScrollAfterInsert; 
    };

    class TraceDrillerLogModel : public QAbstractTableModel
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(TraceDrillerLogModel, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // QAbstractTableModel
        virtual int rowCount(const QModelIndex& index = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& index = QModelIndex()) const;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QVariant data(const QModelIndex& index, int role) const;
        ////////////////////////////////////////////////////////////////////////////////////////////////

        TraceDrillerLogModel(TraceMessageDataAggregator* data, QObject *pParent = NULL);
        virtual ~TraceDrillerLogModel();

    public slots:
        void OnDataCurrentEventChanged();  
        void OnDataAddEvent();
    
    protected:
        TraceMessageDataAggregator*        m_data;
        AZ::s64 m_lastShownEvent;
        QIcon m_criticalIcon;
        QIcon m_errorIcon;
        QIcon m_warningIcon;
        QIcon m_informationIcon;


    };


}

#endif
