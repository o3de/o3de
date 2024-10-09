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
#include <Editor/CollisionGroupWidget.h>
#include <Editor/ConfigurationWindowBus.h>
#include <LyViewPaneNames.h>

namespace PhysX
{
    namespace Editor
    {
        CollisionGroupWidget::CollisionGroupWidget()
        {
        }

        AZ::u32 CollisionGroupWidget::GetHandlerName() const
        {
            return Physics::Edit::CollisionGroupSelector;
        }

        QWidget* CollisionGroupWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            picker->GetEditButton()->setVisible(true);
            picker->GetEditButton()->setToolTip("Edit Collision Groups");

            connect(picker->GetComboBox(), &QComboBox::currentTextChanged, this, [picker]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, picker);
            });

            connect(picker->GetEditButton(), &QToolButton::clicked, this, &CollisionGroupWidget::OnEditButtonClicked);

            return picker;
        }

        bool CollisionGroupWidget::IsDefaultHandler() const
        {
            return true;
        }

        void CollisionGroupWidget::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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

        void CollisionGroupWidget::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, widget_t* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            instance = GetGroupFromName(GUI->GetComboBox()->currentText().toUtf8().data());
        }

        bool CollisionGroupWidget::ReadValuesIntoGUI([[maybe_unused]] size_t index, widget_t* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            GUI->clearElements();

            auto groupNames = GetGroupNames();
            for (auto& layerName : groupNames)
            {
                GUI->Add(layerName);
            }

            GUI->setValue(GetNameFromGroup(instance));
            return false;
        }

        void CollisionGroupWidget::OnEditButtonClicked()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, LyViewPane::PhysXConfigurationEditor);

            // Set to collision groups tab
            ConfigurationWindowRequestBus::Broadcast(&ConfigurationWindowRequests::ShowCollisionGroupsTab);
        }

        AzPhysics::CollisionGroups::Id CollisionGroupWidget::GetGroupFromName(const AZStd::string& groupName)
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            return configuration.m_collisionGroups.FindGroupIdByName(groupName);
        }

        AZStd::string CollisionGroupWidget::GetNameFromGroup(const AzPhysics::CollisionGroups::Id& collisionGroup)
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            return configuration.m_collisionGroups.FindGroupNameById(collisionGroup);
        }

        AZStd::vector<AZStd::string> CollisionGroupWidget::GetGroupNames()
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            const AZStd::vector<AzPhysics::CollisionGroups::Preset>& collisionGroupPresets = configuration.m_collisionGroups.GetPresets();

            AZStd::vector<AZStd::string> groupNames;
            groupNames.reserve(collisionGroupPresets.size());

            for (const auto& preset : collisionGroupPresets)
            {
                groupNames.push_back(preset.m_name);
            }
            return groupNames;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/moc_CollisionGroupWidget.cpp>
