/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QSettings>

#define LUAEDITOR_SETTINGS_RECENT_FILES_KEY        (QString("Recent Files"))
#define LUAEDITOR_SETTINGS_RECENT_FILES_PATH_KEY   (QString("path"))
#define LUAEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX  (10)
#define LUAEDITOR_GROUPNAME "Lua Editor"

#define AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME "O3DE"
#define AZ_QCOREAPPLICATION_SETTINGS_APPLICATION_NAME "O3DE"

QStringList ReadRecentFiles()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

    settings.beginGroup(LUAEDITOR_GROUPNAME);
    int count = std::min(settings.beginReadArray(LUAEDITOR_SETTINGS_RECENT_FILES_KEY), LUAEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX);

    // QSettings -> QStringList.
    QStringList recentFiles;
    {
        for (int i = 0; i < count; ++i)
        {
            settings.setArrayIndex(i);
            recentFiles.append(settings.value(LUAEDITOR_SETTINGS_RECENT_FILES_PATH_KEY).toString());
        }
    }

    settings.endArray();
    settings.endGroup();

    return recentFiles;
}

void WriteRecentFiles(const QStringList& recentFiles)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

    settings.beginGroup(LUAEDITOR_GROUPNAME);
    settings.beginWriteArray(LUAEDITOR_SETTINGS_RECENT_FILES_KEY);
    int count = std::min(recentFiles.size(),
            LUAEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX);

    // QSettings -> QStringList.
    {
        for (int i = 0; i < count; ++i)
        {
            settings.setArrayIndex(i);
            settings.setValue(LUAEDITOR_SETTINGS_RECENT_FILES_PATH_KEY, recentFiles.at(i));
        }
    }

    settings.endArray();
    settings.endGroup();
}

void AddRecentFile(const QString& filename)
{
    // QSettings -> QStringList.
    QStringList recentFiles = ReadRecentFiles();

    recentFiles.prepend(filename);
    recentFiles.removeDuplicates();

    WriteRecentFiles(recentFiles);
}

void ClearRecentFile()
{
    WriteRecentFiles(QStringList());
}
