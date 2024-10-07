/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/CollisionLayerWidget.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/PropertyTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 CollisionLayerWidget::GetHandlerName() const
        {
            return Physics::Edit::CollisionLayerSelector;
        }

        QWidget* CollisionLayerWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            picker->GetEditButton()->setVisible(true);
            picker->GetEditButton()->setToolTip("Edit Collision Layers");

            connect(picker->GetComboBox(), &QComboBox::currentTextChanged, this, [picker]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                        &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, picker);
            });

            connect(picker->GetEditButton(), &QToolButton::clicked, this, &CollisionLayerWidget::OnEditButtonClicked);

            return picker;
        }

        bool CollisionLayerWidget::IsDefaultHandler() const
        {
            return true;
        }

        void CollisionLayerWidget::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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

        void CollisionLayerWidget::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, widget_t* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            instance = GetLayerFromName(GUI->GetComboBox()->currentText().toUtf8().data());
        }

        bool CollisionLayerWidget::ReadValuesIntoGUI([[maybe_unused]] size_t index, widget_t* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            GUI->clearElements();

            auto layerNames = GetLayerNames();
            for (auto& layerName : layerNames)
            {
                GUI->Add(layerName);
            }

            GUI->setValue(GetNameFromLayer(instance));
            return true;
        }

        void CollisionLayerWidget::OnEditButtonClicked()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, LyViewPane::PhysXConfigurationEditor);

            // Set to collision layers tab
            ConfigurationWindowRequestBus::Broadcast(&ConfigurationWindowRequests::ShowCollisionLayersTab);
        }

        AzPhysics::CollisionLayer CollisionLayerWidget::GetLayerFromName(const AZStd::string& layerName)
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            return configuration.m_collisionLayers.GetLayer(layerName);
        }

        AZStd::string CollisionLayerWidget::GetNameFromLayer(const AzPhysics::CollisionLayer& layer)
        {
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;
            return configuration.m_collisionLayers.GetName(layer);
        }

        AZStd::vector<AZStd::string> CollisionLayerWidget::GetLayerNames()
        {
            AZStd::vector<AZStd::string> layerNames;
            const AzPhysics::CollisionConfiguration& configuration = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_collisionConfig;

            for (AZ::u8 layer = 0; layer < AzPhysics::CollisionLayers::MaxCollisionLayers; ++layer)
            {
                const auto& layerName = configuration.m_collisionLayers.GetName(layer);
                if (!layerName.empty())
                {
                    layerNames.push_back(layerName);
                }
            }
            return layerNames;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/moc_CollisionLayerWidget.cpp>
