/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiDynamicLayoutBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This component supports dynamic creation of child elements
class UiDynamicLayoutComponent
    : public AZ::Component
    , public UiDynamicLayoutBus::Handler
    , public UiInitializationBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiElementNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiDynamicLayoutComponent, LyShine::UiDynamicLayoutComponentUuid, AZ::Component);

    UiDynamicLayoutComponent();
    ~UiDynamicLayoutComponent() override;

    // UiDynamicLayoutInterface
    virtual void SetNumChildElements(int numChildren) override;
    // ~UiDynamicLayoutInterface

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiTransformChangeNotificationBus
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    // ~UiTransformChangeNotificationBus

    // UiElementNotifications
    void OnUiElementBeingDestroyed() override;
    // ~UiElementNotifications

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiDynamicContentService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiDynamicContentService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
        required.push_back(AZ_CRC_CE("UiLayoutService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiDynamicLayoutComponent);

    void SetPrototypeElementActive(bool active);

    //! Resize the parent to fit all cloned child elements
    void ResizeToFitChildElements();

protected: // data

    //! The entity Id of the prototype element
    AZ::EntityId m_prototypeElement;

    //! Number of child elements to clone on initialization
    int m_numChildElementsToClone;

    //! Stores the size of the prototype element before it is removed from the child list. Used
    //! to calculate the element size
    AZ::Vector2 m_prototypeElementSize;
};
