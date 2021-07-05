/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GUIOptions.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MCore/Source/Distance.h>
#include "PluginOptionsBus.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>

namespace EMStudio
{
    const char* GUIOptions::s_maxRecentFilesOptionName = "maxRecentFiles";
    const char* GUIOptions::s_maxHistoryItemsOptionName = "maxHistoryItems";
    const char* GUIOptions::s_notificationVisibleTimeOptionName = "notificationVisibleTime";
    const char* GUIOptions::s_autosaveIntervalOptionName = "autosaveInterval";
    const char* GUIOptions::s_autosaveNumberOfFilesOptionName = "autosaveNumberOfFiles";
    const char* GUIOptions::s_enableAutosaveOptionName = "enableAutosave";
    const char* GUIOptions::s_importerLogDetailsEnabledOptionName = "importerLogDetailsEnabled";
    const char* GUIOptions::s_autoLoadLastWorkspaceOptionName = "autoLoadLastWorkspace";
    const char* GUIOptions::s_applicationModeOptionName = "applicationMode";
     
    GUIOptions::GUIOptions()
        : m_maxRecentFiles(16)
        , m_maxHistoryItems(256)
        , m_notificationVisibleTime(5)
        , m_autoSaveInterval(10)
        , m_autoSaveNumberOfFiles(5)
        , m_enableAutoSave(true)
        , m_importerLogDetailsEnabled(false)
        , m_applicationMode("AnimGraph")
    {}

    GUIOptions& GUIOptions::operator=(const GUIOptions& other)
    {
        SetMaxRecentFiles(other.GetMaxRecentFiles());
        SetMaxHistoryItems(other.GetMaxHistoryItems());
        SetNotificationInvisibleTime(other.GetNotificationInvisibleTime());
        SetAutoSaveInterval(other.GetAutoSaveInterval());
        SetAutoSaveNumberOfFiles(other.GetAutoSaveNumberOfFiles());
        SetEnableAutoSave(other.GetEnableAutoSave());
        SetImporterLogDetailsEnabled(other.GetImporterLogDetailsEnabled());
        SetApplicationMode(other.GetApplicationMode());

        return *this;
    }

    void GUIOptions::Save(QSettings& settings, QMainWindow& mainWindow)
    {
        settings.beginGroup("EMotionFX");

        // Save the maximum number of items in the command history
        settings.setValue(s_maxHistoryItemsOptionName, m_maxHistoryItems);

        // Save the notification visible time
        settings.setValue(s_notificationVisibleTimeOptionName, m_notificationVisibleTime);

        // Save the autosave settings
        settings.setValue(s_enableAutosaveOptionName, m_enableAutoSave);
        settings.setValue(s_autosaveIntervalOptionName, m_autoSaveInterval);
        settings.setValue(s_autosaveNumberOfFilesOptionName, m_autoSaveNumberOfFiles);

        // Save the new maximum number of recent files
        settings.setValue(s_maxRecentFilesOptionName, m_maxRecentFiles);

        // Save the log details flag for the importer
        settings.setValue(s_importerLogDetailsEnabledOptionName, m_importerLogDetailsEnabled);

        // Save the last used application mode string
        settings.setValue(s_applicationModeOptionName, m_applicationMode.c_str());

        // Save the auto load last workspace flag
        settings.setValue(s_autoLoadLastWorkspaceOptionName, m_autoLoadLastWorkspace);

        // Main window position
        settings.setValue("mainWindowPosX", mainWindow.pos().x());
        settings.setValue("mainWindowPosY", mainWindow.pos().y());

        // Main window size
        settings.setValue("mainWindowSizeX", mainWindow.size().width());
        settings.setValue("mainWindowSizeY", mainWindow.size().height());

        // Maximized state
        const bool isMaximized = mainWindow.windowState() & Qt::WindowMaximized;
        settings.setValue("mainWindowMaximized", isMaximized);

        settings.endGroup();
    }

    GUIOptions GUIOptions::Load(QSettings& settings, QMainWindow& mainWindow)
    {
        GUIOptions options;
        settings.beginGroup("EMotionFX");

        // Read the maximum number of items in the command history
        QVariant tmpVariant = settings.value(s_maxHistoryItemsOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_maxHistoryItems = tmpVariant.toInt();
        }

        // Read the notification visible time
        tmpVariant = settings.value(s_notificationVisibleTimeOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_notificationVisibleTime = tmpVariant.toInt();
        }
        
        // Read the autosave settings
        tmpVariant = settings.value(s_enableAutosaveOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_enableAutoSave = tmpVariant.toBool();
        }
        tmpVariant = settings.value(s_autosaveIntervalOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_autoSaveInterval = tmpVariant.toInt();
        }
        tmpVariant = settings.value(s_autosaveNumberOfFilesOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_autoSaveNumberOfFiles = tmpVariant.toInt();
        }
        
        // Read the maximum number of recent files
        tmpVariant = settings.value(s_maxRecentFilesOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_maxRecentFiles = tmpVariant.toInt();
        }
        
        // Read the log details flag for the importer
        tmpVariant = settings.value(s_importerLogDetailsEnabledOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_importerLogDetailsEnabled = tmpVariant.toBool();
        }

        // Read the last used application mode string
        tmpVariant = settings.value(s_applicationModeOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_applicationMode = tmpVariant.toString().toUtf8().data();
        }

        // Load the auto load last workspace flag
        tmpVariant = settings.value(s_autoLoadLastWorkspaceOptionName);
        if (!tmpVariant.isNull())
        {
            options.m_autoLoadLastWorkspace = tmpVariant.toBool();
        }
        
        // Set the size
        const int sizeX = settings.value("mainWindowSizeX", 1920).toInt();
        const int sizeY = settings.value("mainWindowSizeY", 1080).toInt();
        mainWindow.resize(sizeX, sizeY);

        // Set the position
        const bool containsPosX = settings.contains("mainWindowPosX");
        const bool containsPosY = settings.contains("mainWindowPosY");
        if ((containsPosX) && (containsPosY))
        {
            const int posX = settings.value("mainWindowPosX", 0).toInt();
            const int posY = settings.value("mainWindowPosY", 0).toInt();
            mainWindow.move(posX, posY);
        }
        else
        {
            QDesktopWidget desktopWidget;
            const QRect primaryScreenRect = QApplication::primaryScreen()->availableGeometry();
            const int posX = (primaryScreenRect.width() / 2) - (sizeX / 2);
            const int posY = (primaryScreenRect.height() / 2) - (sizeY / 2);
            mainWindow.move(posX, posY);
        }

#if !defined(EMFX_EMSTUDIOLYEMBEDDED)

        // Maximized state
        const bool isMaximized = settings.value("mainWindowMaximized", true).toBool();
        if (isMaximized)
        {
            mainWindow.showMaximized();
        }
        else
        {
            mainWindow.showNormal();
        }

#endif // EMFX_EMSTUDIOLYEMBEDDED

        settings.endGroup();

        return options;
    }
    
    void GUIOptions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GUIOptions>()
            ->Version(1)
            ->Field(s_maxRecentFilesOptionName, &GUIOptions::m_maxRecentFiles)
            ->Field(s_maxHistoryItemsOptionName, &GUIOptions::m_maxHistoryItems)
            ->Field(s_notificationVisibleTimeOptionName, &GUIOptions::m_notificationVisibleTime)
            ->Field(s_enableAutosaveOptionName, &GUIOptions::m_enableAutoSave)
            ->Field(s_autosaveIntervalOptionName, &GUIOptions::m_autoSaveInterval)
            ->Field(s_autosaveNumberOfFilesOptionName, &GUIOptions::m_autoSaveNumberOfFiles)
            ->Field(s_importerLogDetailsEnabledOptionName, &GUIOptions::m_importerLogDetailsEnabled)
            ->Field(s_autoLoadLastWorkspaceOptionName, &GUIOptions::m_autoLoadLastWorkspace)
            ->Field(s_applicationModeOptionName, &GUIOptions::m_applicationMode)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }
        
        editContext->Class<GUIOptions>("EMStudio properties", "General Animation Editor properties")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_maxRecentFiles, "Maximum recent files",
                "When opening more than the maximum recent files, the oldest opened file won't be displayed in the recent files menu anymore.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnMaxRecentFilesChangedCallback)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 99)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_maxHistoryItems, "Undo history size",
                "Maximum number of available undo steps.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnMaxHistoryItemsChangedCallback)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 9999)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_notificationVisibleTime, "Notification visible time",
                "Time in seconds the notification window will show after saving a file.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnNotificationVisibleTimeChangedCallback)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 10)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_enableAutoSave, "Enable autosave",
                "Autosave will automatically save in-progress versions of newly created or opened assets.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnEnableAutoSaveChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_autoSaveInterval, "Autosave interval (minutes)",
                "Saves an in-progress version every X minutes if auto-save is enabled.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnAutoSaveIntervalChangedCallback)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 60)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_autoSaveNumberOfFiles, "Autosave number of files",
                "Keep X auto-saved versions before overwriting the oldest.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnAutoSaveNumberOfFilesChangedCallback)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 99)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_importerLogDetailsEnabled, "Importer detailed logging",
                "Enable to show detailed logging for the importer. (This will affect load time significantly).")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnImporterLogDetailsEnabledChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &GUIOptions::m_autoLoadLastWorkspace, "Auto load last workspace",
                "Opens the last workspace when starting the Animation Editor.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GUIOptions::OnAutoLoadLastWorkspaceChangedCallback)
            ;
    }

    void GUIOptions::SetMaxRecentFiles(int maxRecentFiles)
    {
        if (maxRecentFiles != m_maxRecentFiles)
        {
            m_maxRecentFiles = maxRecentFiles;
            OnMaxRecentFilesChangedCallback();
        }
    }

    void GUIOptions::SetMaxHistoryItems(int maxHistoryItems)
    {
        if (maxHistoryItems != m_maxHistoryItems)
        {
            m_maxHistoryItems = maxHistoryItems;
            OnMaxHistoryItemsChangedCallback();
        }
    }

    void GUIOptions::SetNotificationInvisibleTime(int notificationInvisibleTime)
    {
        if (notificationInvisibleTime != m_notificationVisibleTime)
        {
            m_notificationVisibleTime = notificationInvisibleTime;
            OnNotificationVisibleTimeChangedCallback();
        }
    }

    void GUIOptions::SetAutoSaveInterval(int autoSaveInterval)
    {
        if (autoSaveInterval != m_autoSaveInterval)
        {
            m_autoSaveInterval = autoSaveInterval;
            OnAutoSaveNumberOfFilesChangedCallback();
        }
    }

    void GUIOptions::SetAutoSaveNumberOfFiles(int autoSaveNumberOfFiles)
    {
        if (autoSaveNumberOfFiles != m_autoSaveNumberOfFiles)
        {
            m_autoSaveNumberOfFiles = autoSaveNumberOfFiles;
            OnAutoSaveNumberOfFilesChangedCallback();
        }
    }

    void GUIOptions::SetEnableAutoSave(bool enableAutosave)
    {
        if (enableAutosave != m_enableAutoSave)
        {
            m_enableAutoSave = enableAutosave;
            OnEnableAutoSaveChangedCallback();
        }
    }

    void GUIOptions::SetImporterLogDetailsEnabled(bool importerLogDetailsEnabled)
    {
        if (importerLogDetailsEnabled != m_importerLogDetailsEnabled)
        {
            m_importerLogDetailsEnabled = importerLogDetailsEnabled;
            OnImporterLogDetailsEnabledChangedCallback();
        }
    }

    void GUIOptions::SetAutoLoadLastWorkspace(bool autoLoadLastWorkspace)
    {
        if (autoLoadLastWorkspace != m_autoLoadLastWorkspace)
        {
            m_autoLoadLastWorkspace = autoLoadLastWorkspace;
            OnAutoLoadLastWorkspaceChangedCallback();
        }
    }

    void GUIOptions::SetApplicationMode(const AZStd::string& applicationMode)
    {
        if (applicationMode != m_applicationMode)
        {
            m_applicationMode = applicationMode;
            OnApplicationModeChangedCallback();
        }
    }

    void GUIOptions::OnMaxRecentFilesChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_maxRecentFilesOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_maxRecentFilesOptionName);
    }

    void GUIOptions::OnMaxHistoryItemsChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_maxHistoryItemsOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_maxHistoryItemsOptionName);
    }

    void GUIOptions::OnNotificationVisibleTimeChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_notificationVisibleTimeOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_notificationVisibleTimeOptionName);
    }

    void GUIOptions::OnAutoSaveIntervalChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_autosaveIntervalOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_autosaveIntervalOptionName);
    }

    void GUIOptions::OnAutoSaveNumberOfFilesChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_autosaveNumberOfFilesOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_autosaveNumberOfFilesOptionName);
    }

    void GUIOptions::OnEnableAutoSaveChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_enableAutosaveOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_enableAutosaveOptionName);
    }

    void GUIOptions::OnImporterLogDetailsEnabledChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_importerLogDetailsEnabledOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_importerLogDetailsEnabledOptionName);
    }

    void GUIOptions::OnAutoLoadLastWorkspaceChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_autoLoadLastWorkspaceOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_autoLoadLastWorkspaceOptionName);
    }

    void GUIOptions::OnApplicationModeChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_applicationModeOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_applicationModeOptionName);
    }

} // namespace EMStudio
