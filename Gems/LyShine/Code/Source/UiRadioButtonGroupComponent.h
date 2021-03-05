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
#pragma once

#include "UiInteractableComponent.h"

#include <LyShine/Bus/UiRadioButtonGroupBus.h>
#include <LyShine/Bus/UiRadioButtonGroupCommunicationBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiRadioButtonGroupComponent
    : public AZ::Component
    , public UiRadioButtonGroupBus::Handler
    , public UiRadioButtonGroupCommunicationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiRadioButtonGroupComponent, LyShine::UiRadioButtonGroupComponentUuid, AZ::Component);

    UiRadioButtonGroupComponent();
    ~UiRadioButtonGroupComponent() override;

    // UiRadioButtonInterface
    AZ::EntityId GetCheckedRadioButton() override;
    void SetState(AZ::EntityId radioButton, bool isOn) override;
    bool GetAllowUncheck() override;
    void SetAllowUncheck(bool allowUncheck) override;
    void AddRadioButton(AZ::EntityId radioButton) override;
    void RemoveRadioButton(AZ::EntityId radioButton) override;
    bool ContainsRadioButton(AZ::EntityId radioButton) override;
    const LyShine::ActionName& GetChangedActionName() override;
    void SetChangedActionName(const LyShine::ActionName& actionName) override;
    // ~UiRadioButtonInterface

    // UiRadioButtonGroupCommunicationInterface
    bool RegisterRadioButton(AZ::EntityId radioButton) override;
    void UnregisterRadioButton(AZ::EntityId radioButton) override;
    void RequestRadioButtonStateChange(AZ::EntityId radioButton, bool newState) override;
    // ~UiRadioButtonGroupCommunicationInterface

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiRadioButtonGroupService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiRadioButtonGroupService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiRadioButtonGroupComponent);

    //! Internal function with the common code for setting the state of the radio button group
    void SetStateCommon(AZ::EntityId radioButton, bool isOn, bool sendNotifications);

private: // data

    bool m_allowUncheck;

    AZ::EntityId m_checkedEntity;
    AZStd::unordered_set<AZ::EntityId> m_radioButtons;

    LyShine::ActionName m_changedActionName;
};
