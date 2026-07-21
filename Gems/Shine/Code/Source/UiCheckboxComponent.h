/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiInteractableComponent.h"
#include <Shine/Bus/UiCheckboxBus.h>
#include <Shine/Bus/UiInitializationBus.h>
#include <Shine/Bus/UiInteractableBus.h>
#include <Shine/UiComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Rendering/TextureAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCheckboxComponent
    : public UiInteractableComponent
    , public UiCheckboxBus::Handler
    , public UiInitializationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCheckboxComponent, Shine::UiCheckboxComponentUuid, AZ::Component);

    UiCheckboxComponent();
    ~UiCheckboxComponent() override;

    // UiCheckboxInterface
    bool GetState() override;
    void SetState(bool isOn) override;
    bool ToggleState() override;
    StateChangeCallback GetStateChangeCallback() override;
    void SetStateChangeCallback(StateChangeCallback onChange) override;
    void SetCheckedEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetCheckedEntity() override;
    void SetUncheckedEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetUncheckedEntity() override;
    const Shine::ActionName& GetTurnOnActionName() override;
    void SetTurnOnActionName(const Shine::ActionName& actionName) override;
    const Shine::ActionName& GetTurnOffActionName() override;
    void SetTurnOffActionName(const Shine::ActionName& actionName) override;
    const Shine::ActionName& GetChangedActionName() override;
    void SetChangedActionName(const Shine::ActionName& actionName) override;
    // ~UiCheckboxInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiInteractableInterface
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterReleased() override;
    // ~UiInteractableInterface

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiInteractableService"));
        provided.push_back(AZ_CRC_CE("UiStateActionsService"));
        provided.push_back(AZ_CRC_CE("UiNavigationService"));
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

    AZ_DISABLE_COPY_MOVE(UiCheckboxComponent);

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();

    bool HandleReleasedCommon(const AZ::Vector2& point);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    bool m_isOn;

    AZ::EntityId m_optionalCheckedEntity; //!< The optional child element to show when ON.
    AZ::EntityId m_optionalUncheckedEntity; //!< The optional child element to show when OFF.

    StateChangeCallback m_onChange;

    Shine::ActionName m_turnOnActionName;
    Shine::ActionName m_turnOffActionName;
    Shine::ActionName m_changedActionName;
};
