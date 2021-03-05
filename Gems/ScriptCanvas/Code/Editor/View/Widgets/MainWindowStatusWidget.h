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
#include <QWidget>

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <Editor/View/Widgets/ValidationPanel/GraphValidationDockWidgetBus.h>
#endif

namespace Ui
{
    class MainWindowStatusWidget;
}

namespace ScriptCanvasEditor
{
    class MainWindowStatusWidget
        : public QWidget
        , public GraphValidatorDockWidgetNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MainWindowStatusWidget, AZ::SystemAllocator, 0);
        
        MainWindowStatusWidget(QWidget* parent = nullptr);
        ~MainWindowStatusWidget() = default;

        // GraphValidatorDockWidgetNotificationBus
        void OnResultsChanged(int errorCount, int warningCount) override;
        ////
        
    public slots:
        
    signals:
        void OnErrorButtonPressed();
        void OnWarningButtonPressed();
        
    private:
        AZStd::unique_ptr<Ui::MainWindowStatusWidget> m_ui;
    };
}