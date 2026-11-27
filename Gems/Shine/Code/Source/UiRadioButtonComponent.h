/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiInteractableComponent.h"
#include <Shine/Bus/UiRadioButtonBus.h>
#include <Shine/Bus/UiRadioButtonCommunicationBus.h>
#include <Shine/Bus/UiInitializationBus.h>
#include <Shine/Bus/UiInteractableBus.h>
#include <Shine/UiComponentTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiRadioButtonComponent
    : public UiInteractableComponent
    , public UiRadioButtonBus::Handler
    , public UiRadioButtonCommunicationBus::Handler
    , public UiInitializationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiRadioButtonComponent, Shine::UiRadioButtonComponentUuid, AZ::Component);

    UiRadioButtonComponent();
    ~UiRadioButtonComponent() override;

    // UiRadioButtonInterface
    bool GetState() override;
    AZ::EntityId GetGroup() override;
    AZ::EntityId GetCheckedEntity() override;
    void SetCheckedEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetUncheckedEntity() override;
    void SetUncheckedEntity(AZ::EntityId entityId) override;
    const Shine::ActionName& GetTurnOnActionName() override;
    void SetTurnOnActionName(const Shine::ActionName& actionName) override;
    const Shine::ActionName& GetTurnOffActionName() override;
    void SetTurnOffActionName(const Shine::ActionName& actionName) override;
    const Shine::ActionName& GetChangedActionName() override;
    void SetChangedActionName(const Shine::ActionName& actionName) override;
    // ~UiRadioButtonInterface

    // UiRadioButtonCommunicationInterface
    void SetState(bool isOn, bool sendNotifications) override;
    void SetGroup(AZ::EntityId group) override;
    // ~UiRadioButtonCommunicationInterface

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

    AZ_DISABLE_COPY_MOVE(UiRadioButtonComponent);

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();
    EntityComboBoxVec PopulateGroupsEntityList();

    bool HandleReleasedCommon(const AZ::Vector2& point);

private: // data

    bool m_isOn;

    AZ::EntityId m_group;  //!< The group this radio button belongs to.
    AZ::EntityId m_optionalCheckedEntity; //!< The optional child element to show when ON.
    AZ::EntityId m_optionalUncheckedEntity; //!< The optional child element to show when OFF.

    Shine::ActionName m_turnOnActionName;
    Shine::ActionName m_turnOffActionName;
    Shine::ActionName m_changedActionName;
};
