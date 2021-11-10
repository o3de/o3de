/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecentFiles.h"
#include "CommonSettingsConfigurations.h"
#include <AzCore/std/algorithm.h>

#include <QDir>
#include <QSettings>

#define SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_KEY        (QString("Recent Files") + " " + QString::fromLocal8Bit(ScriptCanvasEditor::GetEditingGameDataFolder().c_str()) + "/")
#define SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY   (QString("path"))

namespace ScriptCanvasEditor
{
    QStringList ReadRecentFiles()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        int count = AZStd::min(settings.beginReadArray(SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_KEY),
            static_cast<int>(c_scriptCanvasEditorSettingsRecentFilesCountMax));

        // QSettings -> QStringList.
        QStringList recentFiles;
        {
            for (int i = 0; i < count; ++i)
            {
                settings.setArrayIndex(i);
                QFile f(QDir::toNativeSeparators(settings.value(SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY).toString()));
                if(f.exists())
                {
                    recentFiles.append(f.fileName());
                }
            }
        }

        settings.endArray();
        settings.endGroup();

        return recentFiles;
    }

    void WriteRecentFiles(const QStringList& recentFiles)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(SCRIPTCANVASEDITOR_NAME_SHORT);
        settings.beginWriteArray(SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_KEY);
        int count = AZStd::min(recentFiles.size(),
            static_cast<int>(c_scriptCanvasEditorSettingsRecentFilesCountMax));

        // QSettings -> QStringList.
        {
            for (int i = 0; i < count; ++i)
            {
                settings.setArrayIndex(i);
                QFile f(recentFiles.at(i));
                if(f.exists())
                {
                    settings.setValue(SCRIPTCANVASEDITOR_SETTINGS_RECENT_FILES_PATH_KEY, f.fileName());
                }
            }
        }

        settings.endArray();
        settings.endGroup();
    }

    void AddRecentFile(const QString& filename)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        // QSettings -> QStringList.
        QStringList recentFiles = ReadRecentFiles();

        QFile f(QDir::toNativeSeparators(filename));
        if (f.exists())
        {
            recentFiles.prepend(f.fileName());
            recentFiles.removeDuplicates();
        }

        WriteRecentFiles(recentFiles);
    }

    void ClearRecentFile()
    {
        QStringList empty;
        WriteRecentFiles(QStringList());
    }
}
