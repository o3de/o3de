/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
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
