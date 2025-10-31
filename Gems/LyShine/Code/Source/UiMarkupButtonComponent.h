/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiMarkupButtonBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiInteractableComponent.h"

// Only needed for internal unit-testing
#include <LyShine.h>

#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiMarkupButtonComponent
    : public UiInteractableComponent
    , public UiMarkupButtonBus::Handler
    , public UiClickableTextNotificationsBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiMarkupButtonComponent, LyShine::UiMarkupButtonComponentUuid, UiInteractableComponent);

    UiMarkupButtonComponent();
    ~UiMarkupButtonComponent() override;

    // UiMarkupButtonBus
    AZ::Color GetLinkColor() override;
    void SetLinkColor(const AZ::Color& linkColor) override;
    AZ::Color GetLinkHoverColor() override;
    void SetLinkHoverColor(const AZ::Color& linkHoverColor) override;
    // ~UiMarkupButtonBus

    // UiInteractableInterface
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    // ~UiInteractableInterface

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiClickableTextNotifications
    void OnClickableTextChanged() override;
    // ~UiClickableTextNotifications

public: // static member functions

#if defined(LYSHINE_INTERNAL_UNIT_TEST)

    static void UnitTest(CLyShine* lyshine, IConsoleCmdArgs* cmdArgs);

#endif

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    void UpdateHover();

    void HandleClickableHoverStart(int clickableRectIndex);

    void HandleClickableHoverEnd();

    //! Called when the link color changed
    void OnLinkColorChanged();

    //! Called when the link hover color changed
    void OnLinkHoverColorChanged();

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
        required.push_back(AZ_CRC_CE("UiTextService"));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiMarkupButtonComponent);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    UiClickableTextInterface::ClickableTextRects m_clickableTextRects;  //!< Filter all interactions against clickable text rects.

    AZ::Color m_linkColor = AZ::Color(0.0f, 0.0f, 1.0f, 1.0f); //!< Color to assign to clickable text

    AZ::Color m_linkHoverColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); //!< Hover color for clickable text

    int m_clickableRectHoverIndex = -1; //!< Index of the clickable rect currently be hovered

    int m_clickableRectPressedIndex = -1; //!< Index of the clickable rect that was pressed
};
