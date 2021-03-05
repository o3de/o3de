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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Distance.h>
#include <qglobal.h>

#include "PluginOptions.h"

QT_FORWARD_DECLARE_CLASS(QSettings);
QT_FORWARD_DECLARE_CLASS(QMainWindow);

namespace EMStudio
{
    class MainWindow;

    class GUIOptions : public PluginOptions
    {
        friend class MainWindow;

    public:
        AZ_RTTI(GUIOptions, "{45E3309A-059B-4068-9D59-A1B467EC1C86}", PluginOptions);
        
        static const char* s_maxRecentFilesOptionName;
        static const char* s_maxHistoryItemsOptionName;
        static const char* s_notificationVisibleTimeOptionName;
        static const char* s_autosaveIntervalOptionName;
        static const char* s_autosaveNumberOfFilesOptionName;
        static const char* s_enableAutosaveOptionName;
        static const char* s_importerLogDetailsEnabledOptionName;
        static const char* s_autoLoadLastWorkspaceOptionName;
        static const char* s_applicationModeOptionName;
          
        GUIOptions();
        GUIOptions& operator=(const GUIOptions& other);

        void Save(QSettings& settings, QMainWindow& mainWindow);
        static GUIOptions Load(QSettings& settings, QMainWindow& mainWindow);

        static void Reflect(AZ::ReflectContext* serializeContext);

        int GetMaxRecentFiles() const { return m_maxRecentFiles; }
        void SetMaxRecentFiles(int maxRecentFiles);

        int GetMaxHistoryItems() const { return m_maxHistoryItems; }
        void SetMaxHistoryItems(int maxHistoryItems);

        int GetNotificationInvisibleTime() const { return m_notificationVisibleTime; }
        void SetNotificationInvisibleTime(int notificationInvisibleTime);

        int GetAutoSaveInterval() const { return m_autoSaveInterval; }
        void SetAutoSaveInterval(int autoSaveInterval);

        int GetAutoSaveNumberOfFiles() const { return m_autoSaveNumberOfFiles; }
        void SetAutoSaveNumberOfFiles(int autoSaveNumberOfFiles);

        bool GetEnableAutoSave() const { return m_enableAutoSave; }
        void SetEnableAutoSave(bool enableAutosave);

        bool GetImporterLogDetailsEnabled() const { return m_importerLogDetailsEnabled; }
        void SetImporterLogDetailsEnabled(bool importerLogDetailsEnabled);

        bool GetAutoLoadLastWorkspace() const { return m_autoLoadLastWorkspace; }
        void SetAutoLoadLastWorkspace(bool autoLoadLastWorkspace);

        const AZStd::string& GetApplicationMode() const { return m_applicationMode; }
        void SetApplicationMode(const AZStd::string& applicationMode);

    private:
        void OnMaxRecentFilesChangedCallback() const;
        void OnMaxHistoryItemsChangedCallback() const;
        void OnNotificationVisibleTimeChangedCallback() const;
        void OnAutoSaveIntervalChangedCallback() const;
        void OnAutoSaveNumberOfFilesChangedCallback() const;
        void OnEnableAutoSaveChangedCallback() const;
        void OnImporterLogDetailsEnabledChangedCallback() const;
        void OnAutoLoadLastWorkspaceChangedCallback() const;
        void OnApplicationModeChangedCallback() const;

        int m_maxRecentFiles;
        int m_maxHistoryItems;
        int m_notificationVisibleTime;
        int m_autoSaveInterval;
        int m_autoSaveNumberOfFiles;
        bool m_enableAutoSave;
        bool m_importerLogDetailsEnabled;
        bool m_autoLoadLastWorkspace;
        AZStd::string m_applicationMode;
    };
} // namespace EMStudio