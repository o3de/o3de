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
        AZ_CLASS_ALLOCATOR(LoggingWindow, AZ::SystemAllocator, 0);

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

