/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecentAssetPath.h"
#include "CommonSettingsConfigurations.h"

#include <QSettings>

#define SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY (QString("Recent Open File Location") + " " + QString::fromLocal8Bit(ScriptCanvasEditor::GetEditingGameDataFolder().c_str()) + "/")

namespace ScriptCanvasEditor
{
    SourceHandle ReadRecentAssetId()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
            SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        QString recentOpenFileLocation;

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        recentOpenFileLocation = settings.value(SCRIPTCANVASEDITOR_SETTINGS_RECENT_OPEN_FILE_LOCATION_KEY).toString();
        settings.endGroup();

        return { nullptr, {}, recentOpenFileLocation.toUtf8().constData() };
    }

    void SetRecentAssetId(SourceHandle assetId)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
            SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        AZStd::string guidStr = assetId.Id().ToString<AZStd::string>();

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
