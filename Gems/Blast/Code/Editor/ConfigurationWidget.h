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
#include <Blast/BlastMaterial.h>
#include <Blast/BlastSystemBus.h>
#include <QWidget>
#endif

namespace AzQtComponents
{
    class TabWidget;
}

namespace Blast::Editor
{
    class SettingsWidget;

    /// Widget for editing Blast settings.
    class ConfigurationWidget : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ConfigurationWidget, AZ::SystemAllocator, 0);

        explicit ConfigurationWidget(QWidget* parent = nullptr);
        ~ConfigurationWidget() override;

        void SetConfiguration(const BlastGlobalConfiguration& configuration);

    signals:
        void onConfigurationChanged(const BlastGlobalConfiguration& configuration);

    private:
        BlastGlobalConfiguration m_configuration;

        AzQtComponents::TabWidget* m_tabs;
        SettingsWidget* m_settings;
    };
} // namespace Blast::Editor
