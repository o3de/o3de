/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UiInteractableComponent.h"

#include <LyShine/Bus/UiDropdownBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/TickBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropdownComponent
    : public UiInteractableComponent
    , public UiDropdownBus::Handler
    , public UiInitializationBus::Handler
    , public UiCanvasInputNotificationBus::Handler
    , public UiInteractableNotificationBus::MultiHandler
    , public AZ::TickBus::Handler
{
public: // types

    //! These are the visual states
    enum
    {
        DropdownStateExpanded = UiInteractableStatesInterface::NumStates,
    };


public: // member functions

    AZ_COMPONENT(UiDropdownComponent, LyShine::UiDropdownComponentUuid, AZ::Component);

    UiDropdownComponent();
    ~UiDropdownComponent() override;

    // UiDropdownInterface
    AZ::EntityId GetValue() override;
    void SetValue(AZ::EntityId value) override;
    AZ::EntityId GetContent() override;
    void SetContent(AZ::EntityId content) override;
    bool GetExpandOnHover() override;
    void SetExpandOnHover(bool expandOnHover) override;
    float GetWaitTime() override;
    void SetWaitTime(float waitTime) override;
    bool GetCollapseOnOutsideClick() override;
    void SetCollapseOnOutsideClick(bool collapseOnOutsideClick) override;
    AZ::EntityId GetExpandedParentId() override;
    void SetExpandedParentId(AZ::EntityId expandedParentId) override;
    AZ::EntityId GetTextElement() override;
    void SetTextElement(AZ::EntityId textElement) override;
    AZ::EntityId GetIconElement() override;
    void SetIconElement(AZ::EntityId iconElement) override;
    void Expand() override;
    void Collapse() override;
    const LyShine::ActionName& GetExpandedActionName() override;
    void SetExpandedActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetCollapsedActionName() override;
    void SetCollapsedActionName(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetOptionSelectedActionName() override;
    void SetOptionSelectedActionName(const LyShine::ActionName& actionName) override;
    // ~UiDropdownInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiInteractableInterface
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterReleased() override;
    void HandleHoverStart() override;
    void HandleHoverEnd() override;
    // ~UiInteractableInterface

    // UiInteractableNotifications
    void OnReceivedHoverByNavigatingFromDescendant(AZ::EntityId descendantEntityId) override;
    // ~UiInteractableNotifications

    // UiCanvasInputNotificationBus
    void OnCanvasPrimaryReleased(AZ::EntityId entityId) override;
    void OnCanvasEnterReleased(AZ::EntityId entityId) override;
    void OnCanvasHoverStart(AZ::EntityId entityId) override;
    void OnCanvasHoverEnd(AZ::EntityId entityId) override;
    // ~UiCanvasInputNotificationBus

    // TickBus
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    // ~TickBus

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

    AZ_DISABLE_COPY_MOVE(UiDropdownComponent);

    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();

    void OnExpandedStateActionsChanged();

    void Expand(bool transferHover);
    void Collapse(bool transferHover);

    bool HandleReleasedCommon(const AZ::Vector2& point);
    void HandleCanvasReleasedCommon(AZ::EntityId entityId, bool positionalInput);

    void TransferHoverToDescendant();
    AZ::EntityId FindFirstDescendantInteractable(AZ::EntityId parentEntityId);
    AZ::EntityId CreateContentParentInteractable();
    bool ContentIsAncestor(AZ::EntityId entityId);
    bool ContentIsAncestor(AZ::EntityId entityId, AZ::EntityId contentId);
    AZ::Outcome<void, AZStd::string> ValidateTypeIsEntityId(const AZ::Uuid& valueType);
    AZ::Outcome<void, AZStd::string> ValidatePotentialContent(void* newValue, const AZ::Uuid& valueType);
    AZ::Outcome<void, AZStd::string> ValidatePotentialExpandedParent(void* newValue, const AZ::Uuid& valueType);

    bool IsNavigationSupported();

    // UiInteractableInterface
    UiInteractableStatesInterface::State ComputeInteractableState() override;
    // ~UiInteractableInterface

private: // data

    AZ::EntityId m_value;

    AZ::EntityId m_content;
    bool m_expandOnHover;
    float m_waitTime;
    bool m_collapseOnOutsideClick;
    AZ::EntityId m_expandedParentId;
    AZ::EntityId m_textElement;
    AZ::EntityId m_iconElement;
    StateActions m_expandedStateActions;
    LyShine::ActionName m_expandedActionName;
    LyShine::ActionName m_collapsedActionName;
    LyShine::ActionName m_optionSelectedActionName;

    bool m_expanded;
    AZ::EntityId m_canvasEntityId;
    float m_delayTimer;
    AZ::EntityId m_baseParent;
    LyShine::EntityArray m_submenus;
    bool m_expandedByClick;

    // An interactable that is created when the dropdown is expanded to act as the parent
    // of the content element. The content element needs a parent interactable in order to constrain
    // navigation between the content's descendant iteractables. Since the content element is
    // reparented from the dropdown interactable when expanded, this temporary interactable takes
    // place as the parent interactable.
    AZ::EntityId m_tempContentParentInteractable;
};
