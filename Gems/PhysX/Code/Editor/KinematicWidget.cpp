/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/PropertyTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/KinematicWidget.h>
#include <Editor/ConfigurationWindowBus.h>
#include <LyViewPaneNames.h>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 KinematicWidget::GetHandlerName() const
        {
            return Physics::Edit::KinematicSelector;
        }

        QWidget* KinematicWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            auto comboBox = picker->GetComboBox();
            comboBox->addItem("Default");
            comboBox->addItem("Kinematic");

            picker->GetEditButton()->setToolTip("Open Kinematic Dialog");

            connect(picker->GetEditButton(), &QToolButton::clicked, this, &KinematicWidget::OnEditButtonClicked);

            return picker;
        }

        bool KinematicWidget::IsDefaultHandler() const
        {
            return false;
        }

        void KinematicWidget::ConsumeAttribute(
            widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
        {
            if (attrib == AZ::Edit::Attributes::ReadOnly)
            {
                bool value = false;
                if (attrValue->Read<bool>(value))
                {
                    GUI->setEnabled(!value);
                }
            }
        }

        void KinematicWidget::WriteGUIValuesIntoProperty(
            [[maybe_unused]] size_t index,
            widget_t* GUI,
            property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            bool val = GUI->GetComboBox()->currentIndex() == 1;
            instance = static_cast<property_t>(val);
        }

        bool KinematicWidget::ReadValuesIntoGUI(
            [[maybe_unused]] size_t index,
            widget_t* GUI,
            const property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            bool val = instance;

            auto comboBox = GUI->GetComboBox();
            comboBox->blockSignals(true);
            comboBox->setCurrentIndex(val ? 1 : 0);
            comboBox->blockSignals(false);

            return false;
        }

        void KinematicWidget::OnEditButtonClicked()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(
                &AzToolsFramework::EditorRequests::OpenViewPane, LyViewPane::PhysXConfigurationEditor);

            // Set to collision layers tab
            ConfigurationWindowRequestBus::Broadcast(&ConfigurationWindowRequests::ShowCollisionLayersTab);

          /*     QWidget* mainWindow = nullptr;
            +AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
            + +KinematicDescriptionDialog kinematicDialog("hi", 2, mainWindow);
            +kinematicDialog.exec();
            +
        }
        * /*/

        }
    } // namespace Editor

} // namespace PhysX

#include <Editor/moc_KinematicWidget.cpp>
