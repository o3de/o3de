/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PropertyEditor/PropertyImageAssetCtrl.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <UI/PropertyEditor/Model/AssetCompleterModel.h>

namespace AtomToolsFramework
{
    using namespace AzToolsFramework;

    PropertyImageAssetCtrl::PropertyImageAssetCtrl(QWidget* parent)
        : PropertyAssetCtrl(parent)
    {
    }

    AssetBrowser::AssetSelectionModel PropertyImageAssetCtrl::GetAssetSelectionModel()
    {
        // allows to select two assets which are derieved from ImageAsset
        AZStd::vector<AZ::Data::AssetType> assetTypes;
        assetTypes.push_back(azrtti_typeid<AZ::RPI::AttachmentImageAsset>());
        assetTypes.push_back(azrtti_typeid<AZ::RPI::StreamingImageAsset>());
        auto selectionModel = AssetSelectionModel::AssetTypesSelection(assetTypes, false);

        selectionModel.SetTitle(m_title);
        return selectionModel;
    }

    void PropertyImageAssetCtrl::ConfigureAutocompleter()
    {
        if (m_completerIsConfigured)
        {
            return;
        }

        PropertyAssetCtrl::ConfigureAutocompleter();

        // Use filter from selection model for the auto completer's filter
        AssetSelectionModel selection = GetAssetSelectionModel();
        m_model->SetFilter(selection.GetDisplayFilter());
    }

    bool PropertyImageAssetCtrl::CanAcceptAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const
    {
        if (!assetId.IsValid() || assetType.IsNull())
        {
            return false;
        }
        return (assetType == azrtti_typeid<AZ::RPI::AttachmentImageAsset>()
            || assetType == azrtti_typeid<AZ::RPI::StreamingImageAsset>());
    }

    QWidget* ImageAssetPropertyHandler::CreateGUI(QWidget* parent)
    {
        // This is the same logic as the AssetPropertyHandlerDefault, only we create our own
        // PropertyImageAssetCtrl instead for the GUI widget
        PropertyImageAssetCtrl* newCtrl = aznew PropertyImageAssetCtrl(parent);

        QObject::connect(newCtrl, &PropertyImageAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, newCtrl);
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
        });

        return newCtrl;
    }

    void ImageAssetPropertyHandler::ConsumeAttribute(PropertyImageAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        // Let the AssetPropertyHandlerDefault handle all of the attributes
        AssetPropertyHandlerDefault::ConsumeAttributeInternal(GUI, attrib, attrValue, debugName);
    }

    void ImageAssetPropertyHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, PropertyImageAssetCtrl* GUI, property_t& instance, [[maybe_unused]] InstanceDataNode* node)
    {
        if (!GUI->GetSelectedAssetID().IsValid())
        {
            instance = property_t(AZ::Data::AssetId(), GUI->GetCurrentAssetType(), "");
        }
        else
        {
            auto assetId = GUI->GetSelectedAssetID();
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            instance = property_t(assetInfo.m_assetId, assetInfo.m_assetType, assetInfo.m_relativePath);
        }
    }

    bool ImageAssetPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyImageAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        // Let the AssetPropertyHandlerDefault handle reading values into the GUI
        return AssetPropertyHandlerDefault::ReadValuesIntoGUIInternal(index, GUI, instance, node);
    }

    void ImageAssetPropertyHandler::Register()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew ImageAssetPropertyHandler());
    }
}
