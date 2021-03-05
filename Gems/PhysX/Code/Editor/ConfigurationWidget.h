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

#if !defined(Q_MOC_RUN)
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzFramework/Physics/World.h>
#endif

namespace AzQtComponents
{
    class TabWidget;
}

namespace PhysX
{
    namespace Editor
    {
        class SettingsWidget;
        class CollisionFilteringWidget;
        class PvdWidget;

        /// Widget for editing physx configuration and settings.
        ///
        class ConfigurationWidget
            : public QWidget
            , public ConfigurationWindowRequestBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(ConfigurationWidget, AZ::SystemAllocator, 0);

            explicit ConfigurationWidget(QWidget* parent = nullptr);
            ~ConfigurationWidget() override;

            void SetConfiguration(const PhysX::PhysXSystemConfiguration& physXSystemConfiguration,
                                  const PhysX::Debug::DebugConfiguration& physXDebugConfiguration,
                                  const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

            // ConfigurationWindowRequestBus
            void ShowCollisionLayersTab() override;
            void ShowCollisionGroupsTab() override;
            void ShowGlobalSettingsTab() override;

        signals:
            void onConfigurationChanged(const PhysX::PhysXSystemConfiguration& physXSystemConfiguration,
                                        const PhysX::Debug::DebugConfiguration& physXDebugConfig,
                                        const AzPhysics::SceneConfiguration& defaultSceneConfiguration);

        private:
            AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
            PhysX::PhysXSystemConfiguration m_physXSystemConfiguration;
            PhysX::Debug::DebugConfiguration m_physXDebugConfiguration;

            AzQtComponents::TabWidget* m_tabs;
            SettingsWidget* m_settings;
            CollisionFilteringWidget* m_collisionFiltering;
            PvdWidget* m_pvd;
        };
    } // namespace Editor
} // namespace PhysX
