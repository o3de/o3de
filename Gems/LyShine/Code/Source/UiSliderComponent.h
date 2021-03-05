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
#include <LyShine/Bus/UiSliderBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiSliderComponent
    : public UiInteractableComponent
    , public UiSliderBus::Handler
    , public UiInitializationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiSliderComponent, LyShine::UiSliderComponentUuid, AZ::Component);

    UiSliderComponent();
    ~UiSliderComponent() override;

    // UiSliderInterface
    float GetValue() override;
    void SetValue(float value) override;
    float GetMinValue() override;
    void SetMinValue(float value) override;
    float GetMaxValue() override;
    void SetMaxValue(float value) override;
    float GetStepValue() override;
    void SetStepValue(float step) override;
    ValueChangeCallback GetValueChangingCallback() override;
    void SetValueChangingCallback(ValueChangeCallback onChange) override;
    const LyShine::ActionName& GetValueChangingActionName() override;
    void SetValueChangingActionName(const LyShine::ActionName& actionName) override;
    ValueChangeCallback GetValueChangedCallback() override;
    void SetValueChangedCallback(ValueChangeCallback onChange) override;
    const LyShine::ActionName& GetValueChangedActionName() override;
    void SetValueChangedActionName(const LyShine::ActionName& actionName) override;
    void SetTrackEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetTrackEntity() override;
    void SetFillEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetFillEntity() override;
    void SetManipulatorEntity(AZ::EntityId entityId) override;
    AZ::EntityId GetManipulatorEntity() override;
    // ~UiSliderInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiInteractableInterface
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterPressed(bool& shouldStayActive) override;
    bool HandleAutoActivation() override;
    bool HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys) override;
    void InputPositionUpdate(AZ::Vector2 point) override;
    bool DoesSupportDragHandOff(AZ::Vector2 startPoint) override;
    bool OfferDragHandOff(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float dragThreshold) override;
    void LostActiveStatus() override;
    // ~UiInteractableInterface

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    // UiInteractableComponent
    bool IsAutoActivationSupported() override;
    // ~UiInteractableComponent

    UiInteractableStatesInterface::State ComputeInteractableState() override;

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiInteractableService", 0x1d474c98));
        provided.push_back(AZ_CRC("UiNavigationService"));
        provided.push_back(AZ_CRC("UiStateActionsService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiInteractableService", 0x1d474c98));
        incompatible.push_back(AZ_CRC("UiNavigationService"));
        incompatible.push_back(AZ_CRC("UiStateActionsService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiSliderComponent);

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();

    float GetValueFromPoint(AZ::Vector2 point);
    float GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint);
    bool CheckForDragOrHandOffToParent(AZ::EntityId currentActiveInteractable, AZ::Vector2 startPoint, AZ::Vector2 currentPoint, float childDragThreshold, bool& handOffDone);

    void DoChangedActions();
    void DoChangingActions();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    float m_value;
    float m_minValue;
    float m_maxValue;
    float m_stepValue;

    bool m_isDragging;
    bool m_isActive; // true when interactable can be manipulated by key input

    ValueChangeCallback m_onValueChanged;
    ValueChangeCallback m_onValueChanging;

    LyShine::ActionName m_valueChangedActionName;
    LyShine::ActionName m_valueChangingActionName;

    AZ::EntityId m_trackEntity;
    AZ::EntityId m_fillEntity;
    AZ::EntityId m_manipulatorEntity;
};
