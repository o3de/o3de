/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(MainWindowStatusWidget, AZ::SystemAllocator);
        
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
