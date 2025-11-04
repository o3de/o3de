/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTooltipDisplayBus.h>
#include <LyShine/Bus/UiTooltipDataPopulatorBus.h>
#include <LyShine/Bus/UiTooltipBus.h>
#include <LyShine/UiComponentTypes.h>
#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTooltipComponent
    : public AZ::Component
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiInteractableNotificationBus::Handler
    , public UiCanvasInputNotificationBus::Handler
    , public UiTooltipDisplayNotificationBus::Handler
    , public UiTooltipDataPopulatorBus::Handler
    , public UiTooltipBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiTooltipComponent, LyShine::UiTooltipComponentUuid, AZ::Component);

    UiTooltipComponent();
    ~UiTooltipComponent() override;

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiInteractableNotifications
    void OnHoverStart() override;
    void OnHoverEnd() override;
    void OnPressed() override;
    void OnReleased() override;
    // ~UiInteractableNotifications

    // UiCanvasInputNotifications
    void OnCanvasPrimaryReleased(AZ::EntityId entityId) override;
    // ~UiCanvasInputNotifications

    // UiTooltipDisplayNotifications
    void OnHiding() override;
    void OnHidden() override;
    // ~UiTooltipDisplayNotifications

    // UiTooltipDataPopulatorInterface
    void PushDataToDisplayElement(AZ::EntityId displayEntityId) override;
    // ~UiTooltipDataPopulatorInterface

    // UiTooltipInterface
    AZStd::string GetText() override;
    void SetText(const AZStd::string& text) override;
    // ~UiTooltipInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiTooltipService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiTooltipService"));
        incompatible.push_back(AZ_CRC_CE("UiTooltipDisplayService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
        required.push_back(AZ_CRC_CE("UiInteractableService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiTooltipComponent);

    // Hide the tooltip or cancel it from showing if in delay
    void HideDisplayElement();

    // Handle the tooltip being hidden implicitly or explicitly
    void HandleDisplayElementHidden();

    // Trigger the tooltip for display
    void TriggerTooltip(UiTooltipDisplayInterface::TriggerMode triggerMode);

    // Return true if the tooltip has been triggered for display or is already showing.
    // Return false if tooltip is hiding or is hidden
    bool IsTriggered();

    // Return whether the tooltip has been triggered for display by the specified mode
    bool IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode triggerMode);

    // Get the display element's trigger mode which could have changed after the tooltip
    // was triggered to display and may be different from m_curTriggerMode
    UiTooltipDisplayInterface::TriggerMode GetDisplayElementTriggerMode();

protected: // data

    //! The tooltip text
    AZStd::string m_text;

    // Valid when the tooltip has been triggered to show or is already showing.
    // Invalid when the tooltip is hiding or is hidden
    AZ::EntityId m_curDisplayElementId;

    // The trigger mode that caused the tooltip to currently display
    UiTooltipDisplayInterface::TriggerMode m_curTriggerMode;
};
