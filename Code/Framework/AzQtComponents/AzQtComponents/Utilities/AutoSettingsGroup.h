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

