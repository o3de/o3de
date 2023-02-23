/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QButtonGroup>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <Editor/View/Widgets/LoggingPanel/LiveWindowSession/LiveLoggingDataAggregator.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#endif

namespace Ui
{
    class LoggingWindow;
}
 
namespace ScriptCanvasEditor
{
    class PivotTreeWidget;

    class LoggingWindow
        : public AzQtComponents::StyledDockWidget
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(LoggingWindow, AZ::SystemAllocator);

        LoggingWindow(QWidget* parentWidget = nullptr);
        virtual ~LoggingWindow();

    protected:

        void OnActiveTabChanged(int index);

        void PivotOnEntities();
        void PivotOnGraphs();

    private:

        PivotTreeWidget* GetActivePivotWidget() const;

        AZStd::unique_ptr<Ui::LoggingWindow> m_ui;

        QButtonGroup m_pivotGroup;

        LoggingDataId m_activeDataId;        

        int m_entityPageIndex;
        int m_graphPageIndex;
    };
}

