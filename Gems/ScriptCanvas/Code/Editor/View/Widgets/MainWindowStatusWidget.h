/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QScopedPointer>

#include <AzCore/Memory/SystemAllocator.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <Editor/View/Widgets/ValidationPanel/GraphValidationDockWidgetBus.h>

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
        ~MainWindowStatusWidget() override;

        // GraphValidatorDockWidgetNotificationBus
        void OnResultsChanged(int errorCount, int warningCount) override;
        ////
        
    public slots:
        
    signals:
        void OnErrorButtonPressed();
        void OnWarningButtonPressed();
        
    private:
        QScopedPointer<Ui::MainWindowStatusWidget> m_ui;
    };
}
