/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Widgets/MainWindowStatusWidget.h>
#include <Editor/View/Widgets/ui_MainWindowStatusWidget.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////
    // MainWindowStatusWidget
    ///////////////////////////
    
    MainWindowStatusWidget::MainWindowStatusWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::MainWindowStatusWidget())
    {
        m_ui->setupUi(this);
        
        QObject::connect(m_ui->showErrorButton, &QToolButton::clicked, this, &MainWindowStatusWidget::OnErrorButtonPressed);
        QObject::connect(m_ui->showWarningButton, &QToolButton::clicked, this, &MainWindowStatusWidget::OnWarningButtonPressed);

        GraphValidatorDockWidgetNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        OnResultsChanged(0, 0);
    }

    void MainWindowStatusWidget::OnResultsChanged(int errorCount, int warningCount)
    {
        m_ui->showErrorButton->setText(QString("%1 Errors").arg(errorCount));
        m_ui->showWarningButton->setText(QString("%1 Warnings").arg(warningCount));
    }

#include <Editor/View/Widgets/moc_MainWindowStatusWidget.cpp>
}
