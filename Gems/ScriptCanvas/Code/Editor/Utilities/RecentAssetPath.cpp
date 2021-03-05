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
#include "precompiled.h"

#include "RecentAssetPath.h"
#include "CommonSettingsConfigurations.h"

#include <QSettings>

#define SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY (QString("Recent Open File Location") + " " + QString::fromLocal8Bit(ScriptCanvasEditor::GetEditingGameDataFolder().c_str()) + "/")

namespace ScriptCanvasEditor
{
    AZ::Data::AssetId ReadRecentAssetId()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
            SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        QString recentOpenFileLocation;

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        recentOpenFileLocation = settings.value(SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY).toString();
        settings.endGroup();

        if (recentOpenFileLocation.isEmpty())
        {
            return {};
        }
        AZ::Data::AssetId assetId(recentOpenFileLocation.toUtf8().constData());
        return assetId;
    }

    void SetRecentAssetId(const AZ::Data::AssetId& assetId)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
            SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        AZStd::string guidStr = assetId.m_guid.ToString<AZStd::string>();

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        settings.setValue(SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY, 
            QVariant::fromValue(QString(guidStr.c_str())));
        settings.endGroup();
    }

    void ClearRecentAssetId()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
            SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        settings.remove(SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY);
        settings.endGroup();
    }
}
