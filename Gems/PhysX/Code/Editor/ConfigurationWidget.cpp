/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <QBoxLayout>
#include <Editor/ConfigurationWidget.h>
#include <Editor/SettingsWidget.h>
#include <Editor/CollisionFilteringWidget.h>
#include <Editor/PvdWidget.h>

namespace PhysX
{
    namespace Editor
    {
        ConfigurationWidget::ConfigurationWidget(QWidget* parent)
            : QWidget(parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(this);
            verticalLayout->setContentsMargins(0, 5, 0, 0);
            verticalLayout->setSpacing(0);

            m_tabs = new AzQtComponents::TabWidget(this);
            AzQtComponents::TabWidget::applySecondaryStyle(m_tabs, false);

            m_settings = new SettingsWidget();
            m_collisionFiltering = new CollisionFilteringWidget();
            m_pvd = new PvdWidget();

            m_tabs->addTab(m_settings, "Global Configuration");
            m_tabs->addTab(m_collisionFiltering, "Collision Filtering");
            m_tabs->addTab(m_pvd, "Debugger");

            verticalLayout->addWidget(m_tabs);

            connect(m_settings, &SettingsWidget::onValueChanged,
                this, [this](const PhysX::PhysXSystemConfiguration& physxSystemConfiguration,
                            const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
                             const Debug::DebugDisplayData& debugDisplayData)
            {
                m_physXSystemConfiguration = physxSystemConfiguration;
                m_defaultSceneConfiguration = defaultSceneConfiguration;
                m_physXDebugConfiguration.m_debugDisplayData = debugDisplayData;
                emit onConfigurationChanged(m_physXSystemConfiguration, m_physXDebugConfiguration, m_defaultSceneConfiguration);
            });

            connect(m_collisionFiltering, &CollisionFilteringWidget::onConfigurationChanged,
                this, [this](const AzPhysics::CollisionLayers& layers, const AzPhysics::CollisionGroups& groups)
            {
                m_physXSystemConfiguration.m_collisionConfig.m_collisionLayers = layers;
                m_physXSystemConfiguration.m_collisionConfig.m_collisionGroups = groups;
                emit onConfigurationChanged(m_physXSystemConfiguration, m_physXDebugConfiguration, m_defaultSceneConfiguration);
            });

            connect(m_pvd, &PvdWidget::onValueChanged,
                this, [this](const Debug::PvdConfiguration& configuration)
            {
                m_physXDebugConfiguration.m_pvdConfigurationData = configuration;
                emit onConfigurationChanged(m_physXSystemConfiguration, m_physXDebugConfiguration, m_defaultSceneConfiguration);
            });

            ConfigurationWindowRequestBus::Handler::BusConnect();
        }

        ConfigurationWidget::~ConfigurationWidget()
        {
            ConfigurationWindowRequestBus::Handler::BusDisconnect();
        }

        void ConfigurationWidget::SetConfiguration(
            const PhysX::PhysXSystemConfiguration& physXSystemConfiguration,
            const PhysX::Debug::DebugConfiguration& physXDebugConfiguration,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration)
        {
            m_physXSystemConfiguration = physXSystemConfiguration;
            m_defaultSceneConfiguration = defaultSceneConfiguration;
            m_physXDebugConfiguration = physXDebugConfiguration;
            m_settings->SetValue(m_physXSystemConfiguration, m_defaultSceneConfiguration, m_physXDebugConfiguration.m_debugDisplayData);
            m_collisionFiltering->SetConfiguration(m_physXSystemConfiguration.m_collisionConfig.m_collisionLayers, m_physXSystemConfiguration.m_collisionConfig.m_collisionGroups);
            m_pvd->SetValue(m_physXDebugConfiguration.m_pvdConfigurationData);
        }

        void ConfigurationWidget::ShowCollisionLayersTab()
        {
            const int index = m_tabs->indexOf(m_collisionFiltering);
            m_tabs->setCurrentIndex(index);
            m_collisionFiltering->ShowLayersTab();
        }

        void ConfigurationWidget::ShowCollisionGroupsTab()
        {
            const int index = m_tabs->indexOf(m_collisionFiltering);
            m_tabs->setCurrentIndex(index);
            m_collisionFiltering->ShowGroupsTab();
        }

        void ConfigurationWidget::ShowGlobalSettingsTab()
        {
            const int index = m_tabs->indexOf(m_settings);
            m_tabs->setCurrentIndex(index);
        }

    } // namespace PhysXConfigurationWidget
} // namespace AzToolsFramework

#include <Editor/moc_ConfigurationWidget.cpp>
