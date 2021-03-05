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

#include <PhysX_precompiled.h>
#include <Editor/CollisionGroupWidget.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/PropertyTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

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
            
            picker->GetEditButton()->setToolTip("Edit Collision Groups");

            connect(picker->GetComboBox(), &QComboBox::currentTextChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
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
            QSignalBlocker signalBlocker(GUI->GetComboBox());
            GUI->GetComboBox()->clear();

            auto groupNames = GetGroupNames();
            for (auto& layerName : groupNames)
            {
                GUI->GetComboBox()->addItem(layerName.c_str());
            }

            auto groupName = GetNameFromGroup(instance);
            GUI->GetComboBox()->setCurrentText(groupName.c_str());
            return true;
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
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<Physics::CollisionRequests>::Get()->GetCollisionConfiguration();
            return configuration.m_collisionGroups.FindGroupIdByName(groupName);
        }

        AZStd::string CollisionGroupWidget::GetNameFromGroup(const AzPhysics::CollisionGroups::Id& collisionGroup)
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<Physics::CollisionRequests>::Get()->GetCollisionConfiguration();
            return configuration.m_collisionGroups.FindGroupNameById(collisionGroup);
        }

        AZStd::vector<AZStd::string> CollisionGroupWidget::GetGroupNames()
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<Physics::CollisionRequests>::Get()->GetCollisionConfiguration();
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
