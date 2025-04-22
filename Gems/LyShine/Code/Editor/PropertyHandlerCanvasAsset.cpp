/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/PropertyHandlerCanvasAsset.h>

namespace LyShineEditor
{
    AZ::u32 CanvasAssetPropertyHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("CanvasAssetRef");
    }

    bool CanvasAssetPropertyHandler::IsDefaultHandler() const
    {
        return false;
    }

    QWidget* CanvasAssetPropertyHandler::GetFirstInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget)
    {
        return widget->GetFirstInTabOrder();
    }

    QWidget* CanvasAssetPropertyHandler::GetLastInTabOrder(AzToolsFramework::PropertyAssetCtrl* widget)
    {
        return widget->GetLastInTabOrder();
    }

    void CanvasAssetPropertyHandler::UpdateWidgetInternalTabbing(AzToolsFramework::PropertyAssetCtrl* widget)
    {
        widget->UpdateTabOrder();
    }

    QWidget* CanvasAssetPropertyHandler::CreateGUI(QWidget* parent)
    {
        AzToolsFramework::PropertyAssetCtrl* newCtrl = aznew AzToolsFramework::PropertyAssetCtrl(parent);

        QObject::connect(newCtrl, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            AzToolsFramework::PropertyEditorGUIMessagesBus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessagesBus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
        });

        return newCtrl;
    }

    void CanvasAssetPropertyHandler::ConsumeAttribute(AzToolsFramework::PropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        // Let ConsumeAttributeForPropertyAssetCtrl handle all of the attributes
        AzToolsFramework::ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);
    }

    void CanvasAssetPropertyHandler::WriteGUIValuesIntoProperty(size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        // Let the SimpleAssetPropertyHandlerDefault handle writing the GUI value into the property
        AzToolsFramework::SimpleAssetPropertyHandlerDefault::WriteGUIValuesIntoPropertyInternal(index, GUI, instance, node);
    }

    bool CanvasAssetPropertyHandler::ReadValuesIntoGUI(size_t index, AzToolsFramework::PropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        // Let the SimpleAssetPropertyHandlerDefault handle reading values into the GUI
        return AzToolsFramework::SimpleAssetPropertyHandlerDefault::ReadValuesIntoGUIInternal(index, GUI, instance, node);
    }

    void CanvasAssetPropertyHandler::Register()
    {
        using namespace AzToolsFramework;

        PropertyTypeRegistrationMessageBus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew CanvasAssetPropertyHandler());
    }
}
