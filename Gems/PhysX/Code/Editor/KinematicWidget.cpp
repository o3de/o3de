/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/KinematicWidget.h>
#include <Editor/KinematicDescriptionDialog.h>

#include <AzFramework/Physics/PropertyTypes.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

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

            connect(
                picker,
                &ComboBoxEditButtonPair::valueChanged,
                this,
                [picker]()
                {
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                        &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::RequestWrite, picker);
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                        &AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, picker);
                });

            auto comboBox = picker->GetComboBox();
            comboBox->addItem("Dynamic");
            comboBox->addItem("Kinematic");

            picker->GetEditButton()->setToolTip("Open Type dialog for a detailed description on the motion types");

            connect(picker->GetEditButton(), &QToolButton::clicked, this, &KinematicWidget::OnEditButtonClicked);
            m_widgetComboBox = comboBox;

            return picker;
        }

        void KinematicWidget::WriteGUIValuesIntoProperty(
            [[maybe_unused]] size_t index,
            widget_t* GUI,
            property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            bool value = GUI->GetComboBox()->currentIndex() == 1;
            instance = static_cast<property_t>(value);
        }

        bool KinematicWidget::ReadValuesIntoGUI(
            [[maybe_unused]] size_t index,
            widget_t* GUI,
            [[maybe_unused]] const property_t& instance,
            [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            auto comboBox = GUI->GetComboBox();
            comboBox->blockSignals(true);
            comboBox->setCurrentIndex(instance ? 1 : 0);
            comboBox->blockSignals(false);

            return false;
        }

        void KinematicWidget::OnEditButtonClicked()
        {
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
            KinematicDescriptionDialog kinematicDialog(m_widgetComboBox->currentIndex() == 1, mainWindow);

            int dialogResult = kinematicDialog.exec();
            if (dialogResult == QDialog::Accepted)
            {
                if (kinematicDialog.GetResult())
                {
                    m_widgetComboBox->setCurrentIndex(1);
                }
                else
                {
                    m_widgetComboBox->setCurrentIndex(0);
                }
            }
        }
    } // namespace Editor

} // namespace PhysX

#include <Editor/moc_KinematicWidget.cpp>
