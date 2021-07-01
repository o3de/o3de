/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <StdAfx.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <Editor/ConfigurationWidget.h>
#include <Editor/SettingsWidget.h>
#include <QBoxLayout>

namespace Blast
{
    namespace Editor
    {
        ConfigurationWidget::ConfigurationWidget(QWidget* parent)
            : QWidget(parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(this);
            verticalLayout->setContentsMargins(0, 5, 0, 0);
            verticalLayout->setSpacing(0);

            m_tabs = new AzQtComponents::TabWidget(this);
            AzQtComponents::TabWidget::applySecondaryStyle(m_tabs, false);

            m_settings = new SettingsWidget();

            m_tabs->addTab(m_settings, "Configuration");

            verticalLayout->addWidget(m_tabs);

            connect(
                m_settings, &SettingsWidget::onValueChanged, this,
                [this](const BlastGlobalConfiguration& configuration)
                {
                    m_configuration = configuration;
                    emit onConfigurationChanged(m_configuration);
                });
        }

        ConfigurationWidget::~ConfigurationWidget() {}

        void ConfigurationWidget::SetConfiguration(const BlastGlobalConfiguration& configuration)
        {
            m_configuration = configuration;
            m_settings->SetValue(m_configuration);
        }
    } // namespace Editor
} // namespace Blast

#include <Editor/moc_ConfigurationWidget.cpp>
