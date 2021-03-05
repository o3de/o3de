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

#ifndef BREAKPOINTS_VIEW_H
#define BREAKPOINTS_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>

#include "LUABreakpointTrackerMessages.h"
#endif

#pragma once


class DHBreakpointsWidget : public QTableWidget, LUAEditor::LUABreakpointTrackerMessages::Bus::Handler
{
    Q_OBJECT;
public:
    // CLASS ALLOCATOR INTENIONALLY OMITTED so that we can be factoried by Qt code.
    DHBreakpointsWidget( QWidget * parent = 0 );
    virtual ~DHBreakpointsWidget();

    void CreateContextMenu();
    QAction *actionDeleteSelected;
    QAction *actionDeleteAll;

    //////////////////////////////////////////////////////////////////////////
    //Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
    virtual void BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints);
    virtual void BreakpointHit(const LUAEditor::Breakpoint& breakpoint);
    virtual void BreakpointResume();

private:

    void RemoveRow( int which );

    void CreateBreakpoint(const AZStd::string& debugName, int lineNumber);
    void RemoveBreakpoint(const AZStd::string& debugName, int lineNumber);

    void PullFromContext();
    bool m_PauseUpdates;

    public slots:
        void OnDoubleClicked( const QModelIndex & );
        void DeleteSelected();
        void DeleteAll();
};

#endif
