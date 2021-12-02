/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>

#include <Editor/ui_EditorWindow.h>
#include <Editor/EditorWindow.h>
#include <Editor/ConfigurationWidget.h>
#include <System/PhysXSystem.h>
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace PhysX
{
    namespace Editor
    {

        EditorWindow::EditorWindow(QWidget* parent)
            : QWidget(parent)
            , m_ui(new Ui::EditorWindowClass())
        {
            m_ui->setupUi(this);

            auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
            const auto* physXSystemConfiguration = azdynamic_cast<const PhysX::PhysXSystemConfiguration*>(physicsSystem->GetConfiguration());
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            const PhysX::Debug::DebugConfiguration& physXDebugConfiguration = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get()->GetDebugConfiguration();
            

            m_ui->m_PhysXConfigurationWidget->SetConfiguration(*physXSystemConfiguration, physXDebugConfiguration, defaultSceneConfiguration);
            connect(m_ui->m_PhysXConfigurationWidget, &PhysX::Editor::ConfigurationWidget::onConfigurationChanged, 
                this, &EditorWindow::SaveConfiguration);
        }

        void EditorWindow::RegisterViewClass()
        {
            AzToolsFramework::ViewPaneOptions options;
            options.preferedDockingArea = Qt::LeftDockWidgetArea;
            options.saveKeyName = "PhysXConfiguration";
            options.isPreview = true;
            AzToolsFramework::RegisterViewPane<EditorWindow>(LyViewPane::PhysXConfigurationEditor, LyViewPane::CategoryTools, options);
        }

        void EditorWindow::SaveConfiguration(
            const PhysX::PhysXSystemConfiguration& physXSystemConfiguration,
            const PhysX::Debug::DebugConfiguration& physXDebugConfig,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration)
        {
            auto* physXSystem = GetPhysXSystem();
            if (physXSystem == nullptr)
            {
                AZ_Error("PhysX", false, "Unable to save the PhysX configuration. The PhysXSystem not initialized. Any changes have not been applied.");
                return;
            }

            //update the physx system config if it has changed
            const PhysXSettingsRegistryManager& settingsRegManager = physXSystem->GetSettingsRegistryManager();
            if (physXSystem->GetPhysXConfiguration() != physXSystemConfiguration)
            {
                auto saveCallback = [](const PhysXSystemConfiguration& config, PhysXSettingsRegistryManager::Result result)
                {
                    AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success, "Unable to save the PhysX configuration. Any changes have not been applied.");
                    if (result == PhysXSettingsRegistryManager::Result::Success)
                    {
                        if (auto* physXSystem = GetPhysXSystem())
                        {
                            physXSystem->UpdateConfiguration(&config);
                        }
                    }
                };
                settingsRegManager.SaveSystemConfiguration(physXSystemConfiguration, saveCallback);
            }

            if (physXSystem->GetDefaultSceneConfiguration() != defaultSceneConfiguration)
            {
                auto saveCallback = [](const AzPhysics::SceneConfiguration& config, PhysXSettingsRegistryManager::Result result)
                {
                    AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success, "Unable to save the Default Scene configuration. Any changes have not been applied.");
                    if (result == PhysXSettingsRegistryManager::Result::Success)
                    {
                        if (auto* physXSystem = GetPhysXSystem())
                        {
                            physXSystem->UpdateDefaultSceneConfiguration(config);
                        }
                    }
                };
                settingsRegManager.SaveDefaultSceneConfiguration(defaultSceneConfiguration, saveCallback);
            }

            //Update the debug configuration
            if (auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
            {
                if (physXDebug->GetDebugConfiguration() != physXDebugConfig)
                {
                    auto saveCallback = [](const Debug::DebugConfiguration& config, PhysXSettingsRegistryManager::Result result)
                    {
                        AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success, "Unable to save the PhysX debug configuration. Any changes have not been applied.");
                        if (result == PhysXSettingsRegistryManager::Result::Success)
                        {
                            if (auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
                            {
                                physXDebug->UpdateDebugConfiguration(config);
                            }
                        }
                    };
                    settingsRegManager.SaveDebugConfiguration(physXDebugConfig, saveCallback);
                }
            }
        }
    }
}
#include <Editor/moc_EditorWindow.cpp>
