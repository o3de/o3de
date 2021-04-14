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

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

#include <Editor/PropertyTypes.h>
#include <Editor/MeshNodeHandler.h>

namespace NvCloth
{
    namespace Editor
    {
        AZ::u32 MeshNodeHandler::GetHandlerName() const
        {
            return MeshNodeSelector;
        }

        QWidget* MeshNodeHandler::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            // Set edit button appearance to go to FBX Settings dialog
            picker->GetEditButton()->setToolTip("Open FBX Settings to setup Cloth Modifiers");
            picker->GetEditButton()->setText("");
            picker->GetEditButton()->setEnabled(false);

            connect(picker->GetComboBox(), 
                &QComboBox::currentTextChanged, this,
                [picker]()
                {
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, picker);
                });

            connect(picker->GetEditButton(), 
                &QToolButton::clicked, this,
                [this, picker]()
                {
                    OnEditButtonClicked(picker);
                });

            return picker;
        }

        bool MeshNodeHandler::IsDefaultHandler() const
        {
            return true;
        }

        void MeshNodeHandler::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
        {
            if (attrib == AZ::Edit::UIHandlers::EntityId)
            {
                AZ::EntityId value;
                if (attrValue->Read<AZ::EntityId>(value))
                {
                    GUI->SetEntityId(value);
                }
                else
                {
                    AZ_WarningOnce("MeshNodeHandler", false, "Failed to read 'EntityId' attribute from property '%s'. Expected entity id.", debugName);
                }
            }
            else if (attrib == AZ::Edit::Attributes::StringList)
            {
                AZStd::vector<AZStd::string> value;
                if (attrValue->Read<AZStd::vector<AZStd::string>>(value))
                {
                    QSignalBlocker signalBlocker(GUI->GetComboBox());
                    GUI->GetComboBox()->clear();

                    for (const auto& item : value)
                    {
                        GUI->GetComboBox()->addItem(item.c_str());
                    }

                    bool hasAsset = GetMeshAsset(GUI->GetEntityId()).Get() != nullptr;
                    GUI->GetEditButton()->setEnabled(hasAsset);
                }
                else
                {
                    AZ_WarningOnce("MeshNodeHandler", false, "Failed to read 'StringList' attribute from property '%s'. Expected string vector.", debugName);
                }
            }
        }

        void MeshNodeHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, widget_t* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            instance = GUI->GetComboBox()->currentText().toUtf8().data();
        }

        bool MeshNodeHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, widget_t* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI->GetComboBox());
            GUI->GetComboBox()->setCurrentText(instance.c_str());
            return true;
        }

        void MeshNodeHandler::OnEditButtonClicked(widget_t* GUI)
        {
            AZ::Data::Asset<AZ::Data::AssetData> meshAsset = GetMeshAsset(GUI->GetEntityId());
            if (meshAsset)
            {
                // Open the asset with the preferred asset editor, which for Mesh and Actor Assets it's FBX Settings.
                bool handled = false;
                AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                    &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, meshAsset.GetId(), handled);
            }
        }

        AZ::Data::Asset<AZ::Data::AssetData> MeshNodeHandler::GetMeshAsset(const AZ::EntityId entityId) const
        {
            AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
            AZ::Render::MeshComponentRequestBus::EventResult(
                modelAsset, entityId, &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
            return modelAsset;
        }
    } // namespace Editor
} // namespace NvCloth

#include <Source/Editor/moc_MeshNodeHandler.cpp>
