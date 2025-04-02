/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
namespace AzToolsFramework
{
    class InstanceDataNode;

    namespace AssetEditor
    {
        // External interaction with Asset Editor
        class AssetEditorRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Open Asset Editor (if it is not already open) and create a new asset of type specified, if possible.
            virtual void CreateNewAsset(const AZ::Data::AssetType& assetType, const AZ::Uuid& observerId) = 0;

            virtual AZ::Outcome<bool, AZStd::string> IsAssetDataValid() { return AZ::Success(true); }

            //! Open Asset Editor (if it is not already open) and load asset by reference
            virtual void OpenAssetEditor(const AZ::Data::Asset<AZ::Data::AssetData>& asset) = 0;
            //! Open Asset Editor (if it is not already open) and load asset by id
            virtual void OpenAssetEditorById(const AZ::Data::AssetId assetId) = 0;
        };
        using AssetEditorRequestsBus = AZ::EBus<AssetEditorRequests>;

        class AssetEditorValidationRequests
            : public AZ::EBusTraits
        {
        public:

            using BusIdType = AZ::Data::AssetId;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            virtual void BeforePropertyEdit(AzToolsFramework::InstanceDataNode* node, AZ::Data::Asset<AZ::Data::AssetData> asset) { AZ_UNUSED(node); AZ_UNUSED(asset); }

            virtual void PreAssetSave(AZ::Data::Asset<AZ::Data::AssetData> asset) { AZ_UNUSED(asset); }

            virtual AZ::Outcome<bool, AZStd::string> IsAssetDataValid(const AZ::Data::Asset<AZ::Data::AssetData>& asset) { AZ_UNUSED(asset); return AZ::Success(true); }
        };
        using AssetEditorValidationRequestBus = AZ::EBus<AssetEditorValidationRequests>;

        // Internal interaction with existing Asset Editor widget
        class AssetEditorWidgetRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Creates a new asset of type provided (if possible) in the currently open Asset Editor window.
            virtual void CreateAsset(const AZ::Data::AssetType& assetType, const AZ::Uuid& observerId) = 0;
            //! Saves the asset being edited in the currently open Asset Editor window to the path provided
            virtual void SaveAssetAs(const AZStd::string_view assetPath) = 0;
            //! Opens the asset provided (by reference) in the currently open Asset Editor window
            virtual void OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) = 0;
            //! Opens the asset provided (by id) in the currently open Asset Editor window
            virtual void OpenAssetById(const AZ::Data::AssetId assetId) = 0;
        };
        using AssetEditorWidgetRequestsBus = AZ::EBus<AssetEditorWidgetRequests>;

        class AssetEditorNotifications : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            using BusIdType = AZ::Uuid;

            virtual void OnAssetCreated([[maybe_unused]] const AZ::Data::AssetId& assetId)
            {
            }
        };

        using AssetEditorNotificationsBus = AZ::EBus<AssetEditorNotifications>;
    } // namespace AssetEditor
} // namespace AzToolsFramework

AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorRequests);
AZTF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorValidationRequests);
AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorWidgetRequests);
AZTF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorNotifications);
