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
