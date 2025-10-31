/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef WATCHES_VIEW_H
#define WATCHES_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QObject>
#include <QWidget>
#include <QStandardItem>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include "LUAWatchesDebuggerMessages.h"
#include "LUALocalsTrackerMessages.h"
#include <AzCore/Script/ScriptContextDebug.h>
#include "LUABreakpointTrackerMessages.h"
#endif

#pragma once

class WatchesFilterModel;

enum WatchesItemID
{
    WATCHES_NAMEITEM = QStandardItem::UserType,
    WATCHES_VALUEITEM,
    WATCHES_TYPEITEM,
    WATCHES_NEWITEM,
};

enum WatchesOperatingMode
{
    WATCHES_MODE_GENERAL = 0,
    WATCHES_MODE_LOCALS,
};

enum WatchesOperatingState
{
    WATCHES_STATE_CONNECTED = 0,
    WATCHES_STATE_DISCONNECTED,
};

class WatchesNameItem;

class WatchesDataModel : public QAbstractItemModel
{
    Q_OBJECT;

public:

    WatchesDataModel();
    ~WatchesDataModel();

    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex parent ( const QModelIndex & index ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    virtual bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );

    void SetOperatingMode( WatchesOperatingMode newMode );

    void AddWatch( const AZ::ScriptContextDebug::DebugValue& newData );
    void AddWatch( AZStd::string );
    void RemoveWatch( QModelIndex &index );
    void UpdateMatchingDVs(const AZ::ScriptContextDebug::DebugValue& newData);

    bool IsTypeChangeAllowed( const QModelIndex &index ) const;
    void SetType( const QModelIndex &index, char newType );

protected:
    // the data = an array of LUA tables
    typedef AZStd::vector<AZ::ScriptContextDebug::DebugValue> DVVector;
    DVVector m_DebugValues;

    // the mapping of child to parent, maintained during all data updates
    typedef AZStd::unordered_map<const AZ::ScriptContextDebug::DebugValue*, const AZ::ScriptContextDebug::DebugValue* > ParentContainer;
    
    mutable ParentContainer m_parents;
    mutable bool m_parentsDirty;
    WatchesOperatingMode m_OperatingMode;
    
    void RegenerateParentsMap() const;
    void RegenerateParentsMapRecurse(const AZ::ScriptContextDebug::DebugValue& dv) const;
    const char *SafetyType( char ) const;

    const AZ::ScriptContextDebug::DebugValue *GetDV(const QModelIndex & index) const;
    const QModelIndex GetTopmostIndex( const QModelIndex & index );
    const bool IsRealIndex( const QModelIndex & index ) const;

    void MarkValuesAsOld(AZ::ScriptContextDebug::DebugValue& dv);
    void ApplyNewData(AZ::ScriptContextDebug::DebugValue& original, const AZ::ScriptContextDebug::DebugValue& newvalues);
    void DVRecursePrint(const AZ::ScriptContextDebug::DebugValue& dv, int indent) const;
};

class DHWatchesWidget
    : public AzToolsFramework::QTreeViewWithStateSaving
    , private LUAEditor::LUAWatchesDebuggerMessages::Handler
    , public LUAEditor::LUALocalsTrackerMessages::Handler
    , public LUAEditor::LUABreakpointTrackerMessages::Bus::Handler
{
    Q_OBJECT

public:

    DHWatchesWidget( QWidget * parent = 0 );
    virtual ~DHWatchesWidget();

    void SetOperatingMode( WatchesOperatingMode newMode );

    virtual void WatchesUpdate(const AZ::ScriptContextDebug::DebugValue& topmostDebugReference);

    virtual void OnDebuggerAttached();
    virtual void OnDebuggerDetached();
    virtual void LocalsUpdate(const AZStd::vector<AZStd::string>& vars);
    virtual void LocalsClear();

    //////////////////////////////////////////////////////////////////////////
    //Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
    virtual void BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints){(void)uniqueBreakpoints;}
    virtual void BreakpointHit(const LUAEditor::Breakpoint& breakpoint);
    virtual void BreakpointResume(){}

protected:

    void CaptureVariables();

    virtual void keyPressEvent ( QKeyEvent * event );
    void ForceSelectNewWatch();

    QStandardItemModel m_WatchesModel;
    WatchesOperatingMode m_OperatingMode;
    WatchesOperatingState m_OperatingState;

    WatchesDataModel m_DM;
    WatchesFilterModel* m_pFilterModel;

    QMetaObject::Connection m_dataModelDataChangedConnection;
    QMetaObject::Connection m_dataModelRestConnection;

    void disconnectDataModelUpdate();
    void connectDataModelUpdate();

public Q_SLOTS:

    void OnItemChanged();
    void OnDoubleClicked( const QModelIndex & );
};

#endif
