/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include <QSettings>

#define UICANVASEDITOR_SETTINGS_RECENT_FILES_KEY        (QString("Recent Files") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY   (QString("path"))
#define UICANVASEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX  (10)

QStringList ReadRecentFiles()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);
    int count = std::min(settings.beginReadArray(UICANVASEDITOR_SETTINGS_RECENT_FILES_KEY),
            UICANVASEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX);

    // QSettings -> QStringList.
    QStringList recentFiles;
    {
        for (int i = 0; i < count; ++i)
        {
            settings.setArrayIndex(i);
            recentFiles.append(settings.value(UICANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY).toString());
        }
    }

    settings.endArray();
    settings.endGroup();

    return recentFiles;
}

void WriteRecentFiles(const QStringList& recentFiles)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);
    settings.beginWriteArray(UICANVASEDITOR_SETTINGS_RECENT_FILES_KEY);
    int count = std::min(recentFiles.size(),
            UICANVASEDITOR_SETTINGS_RECENT_FILES_COUNT_MAX);

    // QSettings -> QStringList.
    {
        for (int i = 0; i < count; ++i)
        {
            settings.setArrayIndex(i);
            settings.setValue(UICANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY, recentFiles.at(i));
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
    QStringList empty;
    WriteRecentFiles(QStringList());
}
