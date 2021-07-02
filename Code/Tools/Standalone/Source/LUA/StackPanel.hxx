/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef STACK_VIEW_H
#define STACK_VIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>

#include "LUAStackTrackerMessages.h"
#endif

#pragma once


class DHStackWidget : public QTableWidget, LUAEditor::LUAStackTrackerMessages::Bus::Handler
{
    Q_OBJECT;
public:
    DHStackWidget( QWidget * parent = 0 );
    virtual ~DHStackWidget();

    //////////////////////////////////////////////////////////////////////////
    //Debugger Messages, from the LUAEditor::LUAStackTrackerMessages::Bus
    virtual void StackUpdate(const LUAEditor::StackList& stackList);
    virtual void StackClear();

private:

    void AppendStackEntry(const AZStd::string& debugName, int lineNumber);

    void DeleteAll();

    public slots:
        void OnDoubleClicked( const QModelIndex & );
};

#endif
