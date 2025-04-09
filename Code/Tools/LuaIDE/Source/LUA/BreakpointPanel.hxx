/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef BREAKPOINTS_VIEW_H
#define BREAKPOINTS_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>

#include <QtCore/QObject>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include "LUABreakpointTrackerMessages.h"
#endif

#pragma once

class DHBreakpointsWidget
    : public QTableWidget
    , LUAEditor::LUABreakpointTrackerMessages::Bus::Handler
{
    Q_OBJECT;

public:
    // CLASS ALLOCATOR INTENIONALLY OMITTED so that we can be factoried by Qt code.
    DHBreakpointsWidget(QWidget* parent = 0);
    virtual ~DHBreakpointsWidget();

    //////////////////////////////////////////////////////////////////////////
    // Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
    virtual void BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints);
    virtual void BreakpointHit(const LUAEditor::Breakpoint& breakpoint);
    virtual void BreakpointResume();

private:
    void RemoveRow(int which);

    void CreateBreakpoint(const AZStd::string& debugName, int lineNumber);
    void RemoveBreakpoint(const AZStd::string& debugName, int lineNumber);

    void PullFromContext();
    bool m_PauseUpdates;

private slots:
    void CreateContextMenu(const QPoint& pos);

public slots:
    void OnDoubleClicked(const QModelIndex&);
    void DeleteSelected();
    void DeleteAll();
};

#endif
