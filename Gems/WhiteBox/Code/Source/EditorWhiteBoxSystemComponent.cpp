/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/WhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "EditorWhiteBoxSystemComponent.h"
#include "EditorWhiteBoxComponentMode.h"
#include "WhiteBoxToolApiReflection.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    void EditorWhiteBoxSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        WhiteBox::Reflect(context);
        EditorWhiteBoxComponentMode::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorWhiteBoxSystemComponent, WhiteBoxSystemComponent>()->Version(1);
        }
    }

    template<typename AssetHandlerT, typename AssetT>
    void RegisterAsset(AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>& assetHandlers)
    {
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<AssetT>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequests::AddExtension, AssetHandlerT::AssetFileExtension);

        assetHandlers.emplace_back(AZStd::make_unique<AssetHandlerT>());
    }

    void EditorWhiteBoxSystemComponent::OnActionContextModeRegistrationHook()
    {
        EditorWhiteBoxComponentMode::RegisterActionContextModes();
    }

    void EditorWhiteBoxSystemComponent::OnActionUpdaterRegistrationHook()
    {
        EditorWhiteBoxComponentMode::RegisterActionUpdaters();
    }

    void EditorWhiteBoxSystemComponent::OnActionRegistrationHook()
    {
        EditorWhiteBoxComponentMode::RegisterActions();
    }

    void EditorWhiteBoxSystemComponent::OnActionContextModeBindingHook()
    {
        EditorWhiteBoxComponentMode::BindActionsToModes();
    }

    void EditorWhiteBoxSystemComponent::OnMenuBindingHook()
    {
        EditorWhiteBoxComponentMode::BindActionsToMenus();
    }

    void EditorWhiteBoxSystemComponent::Activate()
    {
        WhiteBoxSystemComponent::Activate();
        RegisterAsset<Pipeline::WhiteBoxMeshAssetHandler, Pipeline::WhiteBoxMeshAsset>(m_assetHandlers);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void EditorWhiteBoxSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        WhiteBoxSystemComponent::Deactivate();
    }

    void EditorWhiteBoxSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }
} // namespace WhiteBox
