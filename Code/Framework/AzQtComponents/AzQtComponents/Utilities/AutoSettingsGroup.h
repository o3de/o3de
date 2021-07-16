/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QSettings>
#include <QString>

namespace AzQtComponents
{
    // Use this class to begin and end a group of settings with scope resolution.
    // 
    class AutoSettingsGroup
    {
    public:
        AutoSettingsGroup(QSettings* settings, const QString& groupName)
            : m_settings(settings)
        {
            m_settings->beginGroup(groupName);
        }

        ~AutoSettingsGroup()
        {
            m_settings->endGroup();
            m_settings->sync();
        }

    private:
        QSettings* m_settings;
    };

} // namespace AzQtComponents

