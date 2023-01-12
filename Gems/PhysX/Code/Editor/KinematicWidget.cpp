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

        KinematicWidget::KinematicWidget()
        {
        }
        AZ::u32 KinematicWidget::GetHandlerName() const
        {
            return Physics::Edit::KinematicSelector;
        }

        QWidget* KinematicWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            picker->GetEditButton()->setToolTip("Edit Collision Layers");

            connect(
                picker->GetComboBox(),
                &QComboBox::currentTextChanged,
                this,
                [picker]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
                });

            connect(picker->GetEditButton(), &QToolButton::clicked, this, &KinematicWidget::OnEditButtonClicked);

            return picker;
        }

        bool KinematicWidget::IsDefaultHandler() const
        {
            return true;
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

        //void KinematicEnumPropertyComboBoxHandler::ConsumeAttribute(
        //    [[maybe_unused]] AzToolsFramework::GenericComboBoxCtrlBase* GUI,
        //    [[maybe_unused]] AZ::u32 attrib,
        //    [[maybe_unused]] AZ::AttributeReader* attrValue,
        //    [[maybe_unused]] const char* debugName)
        //{
        //    if (attrib == AZ_CRC_CE("EditButton"))
        //    {
        //        AZ_Printf("debugging", "helloS")
        //    }
        //}
        
        void KinematicWidget::WriteGUIValuesIntoProperty(
            [[maybe_unused]] size_t index,
            [[maybe_unused]] widget_t* GUI,
            [[maybe_unused]] property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            //instance = static_cast<AzPhysics::Kinematic>(GUI->GetComboBox()->currentIndex());

        }

        bool KinematicWidget::ReadValuesIntoGUI(
            [[maybe_unused]] size_t index,
            widget_t* GUI,
            [[maybe_unused]] const property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI->GetComboBox());
            GUI->GetComboBox()->clear();

            /*uto layerNames = GetLayerNames();
            for (auto& layerName : layerNames)
            {
                GUI->GetComboBox()->addItem(layerName.c_str());
            }*/

            //auto layerName = instance;
            //GUI->GetComboBox()->setCurrentText(layerName.c_str());

            return true;
        }

        void KinematicWidget::OnEditButtonClicked()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(
                &AzToolsFramework::EditorRequests::OpenViewPane, LyViewPane::PhysXConfigurationEditor);

            // Set to collision layers tab
            ConfigurationWindowRequestBus::Broadcast(&ConfigurationWindowRequests::ShowCollisionLayersTab);
        }
    } // namespace Editor

} // namespace PhysX

#include <Editor/moc_KinematicWidget.cpp>
