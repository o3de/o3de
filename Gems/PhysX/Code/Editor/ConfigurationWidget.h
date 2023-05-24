/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
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
            AZ_CLASS_ALLOCATOR(ConfigurationWidget, AZ::SystemAllocator);

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
