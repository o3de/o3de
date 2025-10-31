/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiInteractableComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/TextureAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiButtonComponent
    : public UiInteractableComponent
    , public UiButtonBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiButtonComponent, LyShine::UiButtonComponentUuid, UiInteractableComponent);

    UiButtonComponent();
    ~UiButtonComponent() override;

    // UiInteractableInterface
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterReleased() override;
    // ~UiInteractableInterface

    // UiButtonInterface
    OnClickCallback GetOnClickCallback() override;
    void SetOnClickCallback(OnClickCallback onClick) override;
    const LyShine::ActionName& GetOnClickActionName() override;
    void SetOnClickActionName(const LyShine::ActionName& actionName) override;
    // ~UiButtonInterface


protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

protected: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiInteractableService"));
        provided.push_back(AZ_CRC_CE("UiNavigationService"));
        provided.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiInteractableService"));
        incompatible.push_back(AZ_CRC_CE("UiNavigationService"));
        incompatible.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiButtonComponent);

    bool HandleReleasedCommon(const AZ::Vector2& point);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    OnClickCallback m_onClick;

    LyShine::ActionName m_actionName;
};
