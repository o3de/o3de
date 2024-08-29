/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/UiAssetTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasAssetRefComponent
    : public AZ::Component
    , public UiCanvasRefBus::Handler
    , public UiCanvasAssetRefBus::Handler
    , public UiCanvasManagerNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCanvasAssetRefComponent, "{05BED4D7-E331-4020-9C17-BD3F4CE4DE85}");

    UiCanvasAssetRefComponent();

    // UiCanvasRefInterface
    AZ::EntityId GetCanvas() override;
    // ~UiCanvasRefInterface

    // UiCanvasAssetRefInterface
    AZStd::string GetCanvasPathname() override;
    void SetCanvasPathname(const AZStd::string& pathname) override;
    bool GetIsAutoLoad() override;
    void SetIsAutoLoad(bool isAutoLoad) override;
    bool GetShouldLoadDisabled() override;
    void SetShouldLoadDisabled(bool shouldLoadDisabled) override;

    AZ::EntityId LoadCanvas() override;
    void UnloadCanvas() override;
    // ~UiCanvasAssetRefInterface

    // UiCanvasManagerNotification
    void OnCanvasUnloaded(AZ::EntityId canvasEntityId) override;
    // ~UiCanvasManagerNotification

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiCanvasRefService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiCanvasRefService"));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    void LaunchUIEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&);

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiCanvasAssetRefComponent);

protected: // data

    //! Persistent properties
    AzFramework::SimpleAssetReference<LyShine::CanvasAsset> m_canvasAssetRef;
    bool m_isAutoLoad;
    bool m_shouldLoadDisabled;

    //! The UI Canvas that is associated with this component entity
    AZ::EntityId m_canvasEntityId;
};
