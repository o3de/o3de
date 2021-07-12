/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <StdAfx.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <LyViewPaneNames.h>

#include <Editor/ConfigurationWidget.h>
#include <Editor/EditorWindow.h>
#include <Editor/ui_EditorWindow.h>

#include <Blast/BlastSystemBus.h>

namespace Blast
{
    namespace Editor
    {
        EditorWindow::EditorWindow(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::EditorWindowClass())
        {
            m_ui->setupUi(this);

            const BlastGlobalConfiguration& configuration =
                AZ::Interface<Blast::BlastSystemRequests>::Get()->GetGlobalConfiguration();

            m_ui->m_BlastConfigurationWidget->SetConfiguration(configuration);
            connect(
                m_ui->m_BlastConfigurationWidget, &Blast::Editor::ConfigurationWidget::onConfigurationChanged, this,
                &EditorWindow::SaveConfiguration);
        }

        void EditorWindow::RegisterViewClass()
        {
            AzToolsFramework::ViewPaneOptions options;
            options.preferedDockingArea = Qt::LeftDockWidgetArea;
            options.saveKeyName = "BlastConfiguration";
            options.isPreview = true;
            AzToolsFramework::RegisterViewPane<EditorWindow>(
                "Blast Configuration (Experimental)", LyViewPane::CategoryTools, options);
        }

        void EditorWindow::SaveConfiguration(const BlastGlobalConfiguration& configuration)
        {
            AZ::Interface<Blast::BlastSystemRequests>::Get()->SetGlobalConfiguration(configuration);
        }
    } // namespace Editor
} // namespace Blast
#include <Editor/moc_EditorWindow.cpp>
