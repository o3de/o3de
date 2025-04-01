/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RPIUtils.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/UI/PropertyEditor/Model/AssetCompleterModel.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/EditorStreamingImageAssetCtrl.h>

namespace GradientSignal
{
    namespace Internal
    {
        bool IsImageDataPixelAPISupportedForAsset(const AZ::Data::AssetId& assetId)
        {
            auto streamingImageAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::StreamingImageAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
            streamingImageAsset.BlockUntilLoadComplete();

            // Verify if the streaming image asset has a pixel format that is supported
            // for the image data pixel retrieval API
            AZ::RHI::Format format = streamingImageAsset->GetImageDescriptor().m_format;
            return AZ::RPI::IsImageDataPixelAPISupported(format);
        }
    }

    SupportedImageAssetPickerDialog::SupportedImageAssetPickerDialog(AssetSelectionModel& selection, QWidget* parent)
        : AzToolsFramework::AssetBrowser::AssetPickerDialog(selection, parent)
    {
    }


    bool SupportedImageAssetPickerDialog::EvaluateSelection() const
    {
        using namespace AzToolsFramework::AssetBrowser;
        if (!AssetPickerDialog::EvaluateSelection())
        {
            return false;
        }
        // note that this function is called as part of the internals of the asset picker dialog
        // and that the selection currently selected refers to the actual entry picked in the UI by the user
        // and not necessarily the products of that file.  The user could have clicked on a source file, a folder, etc.

        // If we have a valid selection (a streaming image asset), we need to also verify
        // that its pixel format is supported by the image data retrieval API
        const AssetBrowserEntry* entry = m_selection.GetResult();
        if (!entry)
        {
            return false;
        }

        if ((entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source) && (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Product))
        {
            return false;
        }

        bool foundValidImage = false;
        entry->VisitDown( // checks itself, and all its children.
            [&](const auto& currentEntry)
            {
                if (const auto productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(currentEntry))
                {
                    if (productEntry->GetAssetType() == AZ::AzTypeInfo<AZ::RPI::StreamingImageAsset>::Uuid())
                    {
                        if (Internal::IsImageDataPixelAPISupportedForAsset(productEntry->GetAssetId()))
                        {
                            foundValidImage = true;
                            return false; // returning false from the visitor stops it from continuing to search.
                        }
                    }
                }
                return true; // continue searching for more...
            });

        return foundValidImage;
    }

    StreamingImagePropertyAssetCtrl::StreamingImagePropertyAssetCtrl(QWidget* parent)
        : AzToolsFramework::PropertyAssetCtrl(parent)
    {
    }

    void StreamingImagePropertyAssetCtrl::PickAssetSelectionFromDialog(AssetSelectionModel& selection, QWidget* parent)
    {
        // We need to override and use our own picker dialog so that we can
        // disable the OK button if a streaming image asset with an unsupported
        // format has been selected
        SupportedImageAssetPickerDialog dialog(selection, parent);
        dialog.exec();
    }

    bool StreamingImagePropertyAssetCtrl::CanAcceptAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const
    {
        using namespace AzToolsFramework::AssetBrowser;
        if (!PropertyAssetCtrl::CanAcceptAsset(assetId, assetType))
        {
            return false;
        }

        // If the asset is a streaming image asset, we need to verify that its pixel format
        // is supported by the image data retrieval API
        if (assetType == AZ::AzTypeInfo<AZ::RPI::StreamingImageAsset>::Uuid())
        {
            return Internal::IsImageDataPixelAPISupportedForAsset(assetId);
        }

        return false;
    }

    void StreamingImagePropertyAssetCtrl::OnAutocomplete(const QModelIndex& index)
    {
        auto assetId = m_model->GetAssetIdFromIndex(GetSourceIndex(index));

        // Only allow the autocompleter to select an asset if it has
        // a supported pixel format
        if (Internal::IsImageDataPixelAPISupportedForAsset(assetId))
        {
            SetSelectedAssetID(assetId);
        }
    }

    void StreamingImagePropertyAssetCtrl::UpdateAssetDisplay()
    {
        AzToolsFramework::PropertyAssetCtrl::UpdateAssetDisplay();

        // If there is a valid asset selected but it's not a supported pixel format,
        // show the error message state for this property
        if (m_selectedAssetID.IsValid() && !Internal::IsImageDataPixelAPISupportedForAsset(m_selectedAssetID))
        {
            UpdateErrorButtonWithMessage(
                AZStd::string::format("Image asset (%s) has an unsupported pixel format", GetCurrentAssetHint().c_str())
            );
        }
    }

    AZ::TypeId StreamingImagePropertyHandler::GetHandledType() const
    {
        return AZ::GetAssetClassId();
    }

    AZ::u32 StreamingImagePropertyHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("GradientSignalStreamingImageAsset");
    }

    bool StreamingImagePropertyHandler::IsDefaultHandler() const
    {
        // We don't this to be registered as a default handler, because we don't want
        // any other AZ::RPI::StreamingImageAsset fields using this handler.
        // We only want this handler to be used if it was explicitly requested by name,
        // which in this case is for the image gradient asset since it needs to validate
        // the format is supported by the pixel retrieval API.
        return false;
    }

    QWidget* StreamingImagePropertyHandler::GetFirstInTabOrder(StreamingImagePropertyAssetCtrl* widget)
    {
        return widget->GetFirstInTabOrder();
    }

    QWidget* StreamingImagePropertyHandler::GetLastInTabOrder(StreamingImagePropertyAssetCtrl* widget)
    {
        return widget->GetLastInTabOrder();
    }

    void StreamingImagePropertyHandler::UpdateWidgetInternalTabbing(StreamingImagePropertyAssetCtrl* widget)
    {
        widget->UpdateTabOrder();
    }

    QWidget* StreamingImagePropertyHandler::CreateGUI(QWidget* parent)
    {
        // This is the same logic as the AssetPropertyHandlerDefault, only we create our own
        // StreamingImagePropertyAssetCtrl instead for the GUI widget
        StreamingImagePropertyAssetCtrl* newCtrl = aznew StreamingImagePropertyAssetCtrl(parent);

        QObject::connect(newCtrl, &StreamingImagePropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
        {
            (void)newAssetID;
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
        });

        return newCtrl;
    }

    void StreamingImagePropertyHandler::ConsumeAttribute(StreamingImagePropertyAssetCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        // Let ConsumeAttributeForPropertyAssetCtrl handle all of the attributes
        AzToolsFramework::ConsumeAttributeForPropertyAssetCtrl(GUI, attrib, attrValue, debugName);
    }

    void StreamingImagePropertyHandler::WriteGUIValuesIntoProperty(size_t index, StreamingImagePropertyAssetCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        // Let the AssetPropertyHandlerDefault handle writing the GUI value into the property
        AzToolsFramework::AssetPropertyHandlerDefault::WriteGUIValuesIntoPropertyInternal(index, GUI, instance, node);
    }

    bool StreamingImagePropertyHandler::ReadValuesIntoGUI(size_t index, StreamingImagePropertyAssetCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        // Let the AssetPropertyHandlerDefault handle reading values into the GUI
        return AzToolsFramework::AssetPropertyHandlerDefault::ReadValuesIntoGUIInternal(index, GUI, instance, node);
    }

    AZ::Data::Asset<AZ::Data::AssetData>* StreamingImagePropertyHandler::CastTo(void* instance, const AzToolsFramework::InstanceDataNode* node, [[maybe_unused]] const AZ::Uuid& fromId, [[maybe_unused]] const AZ::Uuid& toId) const
    {
        // Let the AssetPropertyHandlerDefault handle the downcast
        return AzToolsFramework::AssetPropertyHandlerDefault::CastToInternal(instance, node);
    }

    void StreamingImagePropertyHandler::Register()
    {
        using namespace AzToolsFramework;

        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew StreamingImagePropertyHandler());
    }
}
