/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Shine/IDraw2d.h>
#include <Shine/Bus/UiCanvasBus.h>
#include <Shine/Bus/UiTransform2dBus.h>
#include <Shine/Bus/UiImageBus.h>
#include <Shine/Bus/UiButtonBus.h>

#include <ShineExamples/ShineExamplesCppExampleBus.h>


namespace ShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class for demonstrating how to programmatically create a canvas from scratch in C++
    //! The created canvas shows a few examples of interactable elements (a button, a checkbox, and a
    //! textInput) as well as a very simple example of custom behavior (a small health system with a
    //! health bar that can be damaged / healed through two buttons).
    class ShineExamplesCppExample
        : protected ShineExamplesCppExampleBus::Handler
        , public UiButtonNotificationBus::MultiHandler
    {
    public: // member functions

        ShineExamplesCppExample();
        ~ShineExamplesCppExample();

        // ShineExamplesCppExampleBus
        void CreateCanvas() override;
        void DestroyCanvas() override;
        // ~ShineExamplesCppExampleBus

        // UiButtonNotificationBus
        void OnButtonClick() override;
        // ~UiButtonNotificationBus

        static void Reflect(AZ::ReflectContext* context);

    private: // member functions

        AZ_DISABLE_COPY_MOVE(ShineExamplesCppExample);

        //! Create the background image
        AZ::EntityId CreateBackground();

        //! Create elements from the ground up
        void CreateElementsExample(AZ::EntityId foregroundId);

        //! Create elements with programmatic behavior
        void CreateBehaviorExample(AZ::EntityId foregroundId);

        //! Creates a component
        void CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId);

        //! Creates a button element
        AZ::EntityId CreateButton(const char* name, bool atRoot, AZ::EntityId parent,
            UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
            const char* text, AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor,
            AZ::Color textColor, UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode);

        //! Creates a checkbox element
        AZ::EntityId CreateCheckbox(const char* name, bool atRoot, AZ::EntityId parent,
            UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
            const char* text, AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor,
            AZ::Color checkColor, AZ::Color textColor,
            UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode = UiTransformInterface::ScaleToDeviceMode::None);

        //! Creates a text element
        AZ::EntityId CreateText(const char* name, bool atRoot, AZ::EntityId parent,
            UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
            const char* text, AZ::Color textColor, IDraw2d::HAlign hAlign, IDraw2d::VAlign vAlign,
            UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode = UiTransformInterface::ScaleToDeviceMode::None);

        //! Creates a text input element
        AZ::EntityId CreateTextInput(const char* name, bool atRoot, AZ::EntityId parent,
            UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
            const char* text, const char* placeHolderText,
            AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor,
            AZ::Color textColor, AZ::Color placeHolderColor,
            UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode = UiTransformInterface::ScaleToDeviceMode::None);

        //! Creates an image element
        AZ::EntityId CreateImage(const char* name, bool atRoot, AZ::EntityId parent,
            UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
            AZStd::string spritePath, UiImageInterface::ImageType imageType, AZ::Color color,
            UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode = UiTransformInterface::ScaleToDeviceMode::None);

        //! Change the health by change amount and update the health bar
        void UpdateHealth(int change);

    private: // data

        AZ::EntityId m_canvasId;

        AZ::EntityId m_damageButton;
        AZ::EntityId m_healButton;
        AZ::EntityId m_destroyButton;

        AZ::EntityId m_healthBar;
        UiTransform2dInterface::Offsets m_maxHealthBarOffsets;
        int m_health;
    };
}
