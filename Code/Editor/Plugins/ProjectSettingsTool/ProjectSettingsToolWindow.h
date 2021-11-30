/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "LastPathBus.h"
#include "Platforms.h"
#include "PlatformSettings.h"
#include "ProjectSettingsContainer.h"
#include "ProjectSettingsSerialization.h"
#include "ValidatorBus.h"

#include <QProcess>
#include <QScopedPointer>
#include <QWidget>
#endif

// Forward Declares
namespace Ui
{
    class ProjectSettingsToolWidget;
}

namespace AzToolsFramework
{
    class InstanceDataHierarchy;
    class PropertyHandlerBase;
    class ReflectedPropertyEditor;
}

namespace ProjectSettingsTool
{
    //Forward Declares
    class Validator;
    class ValidationHandler;
    class PropertyLinkedHandler;

    struct Properties
    {
        BaseSettings base;
        AndroidSettings android;
        IosSettings ios;
    };

    // Main window for Project Settings tool
    class ProjectSettingsToolWindow
        : public QWidget
        , public LastPathBus::Handler
        , public ValidatorBus::Handler

    {
        Q_OBJECT
    public:
        static const GUID& GetClassID()
        {
            // {0DC1B7D9-B660-41C3-91F1-A643EE65AADF}
            static const GUID guid = {
                0x0dc1b7d9, 0xb660, 0x41c3, { 0x91, 0xf1, 0xa6, 0x43, 0xee, 0x65, 0xaa, 0xdf }
            };
            return guid;
        }

        ProjectSettingsToolWindow(QWidget* parent = nullptr);
        ~ProjectSettingsToolWindow();

        // Ebuses
        QString GetLastImagePath() override;
        void SetLastImagePath(const QString& path) override;

        FunctorValidator* GetValidator(FunctorValidator::FunctorType) override;
        void TrackValidator(FunctorValidator*) override;

        static void ReflectPlatformClasses();

    private:
        AZ_DISABLE_COPY_MOVE(ProjectSettingsToolWindow);

        void closeEvent(QCloseEvent* event) override;
        // Close the window now because an error occurred
        void ForceClose();

        // Un/Registers and dis/connect handlers and buses
        void RegisterHandlersAndBusses();
        void UnregisterHandlersAndBusses();

        void InitializeUi();

        // Used to make the serializers for settings
        void MakeSerializerJson(const Platform& plat, AzToolsFramework::InstanceDataHierarchy& hierarchy, rapidjson::Document* doc);
        void MakeSerializerJsonNonRoot(const Platform& plat, AzToolsFramework::InstanceDataHierarchy& hierarchy, rapidjson::Document* doc, rapidjson::Value* jsonRoot);
        void MakeSerializerPlist(const Platform& plat, AzToolsFramework::InstanceDataHierarchy& hierarchy, PlistDictionary* dict);

        // Shows an error dialog if an error has occurred while loading settings then exits if users chooses
        // Returns true if there was an error
        bool IfErrorShowThenExit();
        // Loop through all errors then exit if the user chooses to abort or window is in invalid state
        void ShowAllErrorsThenExitIfInvalid();

        // Resizes TabWidget to size of current tab instead of largest tab
        void ResizeTabs(int index);

        // Add all platforms into the ui
        void AddAllPlatformsToUi();
        // Add given platform into the ui
        void AddPlatformToUi(const Platform& plat);
        // Makes all serializers
        void MakeSerializers();
        // Makes the serializer for specified platform
        void MakePlatformSerializer(const Platform& plat);

        // Replace values in ui with those read from settings
        void LoadPropertiesFromSettings();
        // Load properties for specified platform from file
        void LoadPropertiesFromPlatformSettings(const Platform& plat);

        // Checks if ui is the same as all settings
        bool UiEqualToSettings();
        // Checks if platform is the same as settings
        bool UiEqualToPlatformSettings(const Platform& plat);

        // Checks if all properties are valid, if any are not returns false, also sets warnings on those properties
        bool ValidateAllProperties();

        // Replace values in settings with those from ui and save to file
        void SaveSettingsFromUi();
        // Saves all properties for specified platform from to file
        void SaveSettingsFromPlatformUi(const Platform& plat);

        // Reload settings files and replace the values in ui with them
        void ReloadUiFromSettings();

        // returns true if the platform is enabled
        bool PlatformEnabled(PlatformId platformId);

        // returns the main platform specific resource file e.g. for iOS it would be the Info.plist
        AZStd::string GetPlatformResource(PlatformId platformId);

        // The ui for the window
        QScopedPointer<Ui::ProjectSettingsToolWidget> m_ui;

        // The process used to reconfigure settings
        QProcess m_reconfigureProcess;

        AZStd::string m_projectRoot;
        AZStd::string m_projectName;

        // Used to initialize the settings container's pLists
        ProjectSettingsContainer::PlistInitVector m_plistsInitVector;

        // Container to manage settings files per platform
        AZStd::unique_ptr<ProjectSettingsContainer> m_settingsContainer;
        // Allows lookup and contains all allocated QValidators
        AZStd::unique_ptr<Validator> m_validator;

        Properties m_platformProperties;
        AzToolsFramework::ReflectedPropertyEditor* m_platformPropertyEditors[static_cast<unsigned>(PlatformId::NumPlatformIds)];
        AZStd::unique_ptr<Serializer> m_platformSerializers[static_cast<unsigned>(PlatformId::NumPlatformIds)];

        // Pointers to all handlers to they can be unregistered and deleted
        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> m_propertyHandlers;
        AZStd::unique_ptr<ValidationHandler> m_validationHandler;
        PropertyLinkedHandler* m_linkHandler;

        // Last path used when browsing for images in icons or splash
        QString m_lastImagesPath;
        bool m_invalidState;
    };
} // namespace ProjectSettingsTool
