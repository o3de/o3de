/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShineExamples_precompiled.h"

#include "LyShineExamplesCppExample.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCheckboxBus.h>
#include <LyShine/Bus/UiSliderBus.h>
#include <LyShine/Bus/UiTextInputBus.h>
#include <LyShine/Bus/UiInteractableStatesBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/Bus/UiAnimationBus.h>
#include <LyShine/Bus/UiNavigationBus.h>

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LyShineExamplesCppExample::LyShineExamplesCppExample()
        : m_health(10)
    {
        LyShineExamplesCppExampleBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LyShineExamplesCppExample::~LyShineExamplesCppExample()
    {
        LyShineExamplesCppExampleBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::CreateCanvas()
    {
        // Remove the existing example canvas if it exists
        DestroyCanvas();

        AZ::EntityId canvasEntityId = gEnv->pLyShine->CreateCanvas();
        if (!canvasEntityId.IsValid())
        {
            return;
        }
        m_canvasId = canvasEntityId;

        // Create an image to be the canvas background
        AZ::EntityId foregroundId = CreateBackground();

        // Create the canvas title
        CreateText("Title", false, foregroundId, UiTransform2dInterface::Anchors(0.5f, 0.1f, 0.5f, 0.1f), UiTransform2dInterface::Offsets(-200, 50, 200, -50),
            "Canvas created through C++", AZ::Color(0.f, 0.f, 0.f, 1.f), IDraw2d::HAlign::Center, IDraw2d::VAlign::Center,
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);

        // Add the elements examples, creating elements from scratch
        CreateElementsExample(foregroundId);

        // Add the behavior example, creating some light defining of custom behavior in C++
        CreateBehaviorExample(foregroundId);

        // Create a button to be able to destroy this canvas and keep navigating the UiFeatures examples
        m_destroyButton = CreateButton("DestroyButton", false, foregroundId, UiTransform2dInterface::Anchors(0.15f, 0.9f, 0.15f, 0.9f), UiTransform2dInterface::Offsets(-100, -25, 100, 25),
            "Destroy canvas", AZ::Color(0.604f, 0.780f, 0.839f, 1.f), AZ::Color(0.380f, 0.745f, 0.871f, 1.f), AZ::Color(0.055f, 0.675f, 0.886f, 1.f), AZ::Color(1.f, 1.f, 1.f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        // Connect to the button notification bus so we receive click events from the destroy button
        UiButtonNotificationBus::MultiHandler::BusConnect(m_destroyButton);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::DestroyCanvas()
    {
        if (m_canvasId.IsValid())
        {
            UiButtonNotificationBus::MultiHandler::BusDisconnect(m_damageButton);
            m_damageButton.SetInvalid();
            UiButtonNotificationBus::MultiHandler::BusDisconnect(m_healButton);
            m_healButton.SetInvalid();
            UiButtonNotificationBus::MultiHandler::BusDisconnect(m_destroyButton);
            m_destroyButton.SetInvalid();

            m_healthBar.SetInvalid();

            gEnv->pLyShine->ReleaseCanvas(m_canvasId, false);
            m_canvasId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::OnButtonClick()
    {
        // Get the id of what button just got clicked (it has to be one that we subscribed to)
        const AZ::EntityId* pButtonClickedId = UiButtonNotificationBus::GetCurrentBusId();

        // If the damage button got clicked
        if (*pButtonClickedId == m_damageButton)
        {
            UpdateHealth(-1);
        }
        // If the heal button got clicked
        else if (*pButtonClickedId == m_healButton)
        {
            UpdateHealth(1);
        }
        // If the destroy button got clicked
        else // *pButtonClickedId == m_destroyButton
        {
            DestroyCanvas();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::Reflect(AZ::ReflectContext* context)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<LyShineExamplesCppExampleBus>("LyShineExamplesCppExampleBus")
                ->Event("CreateCanvas", &LyShineExamplesCppExampleBus::Events::CreateCanvas)
                ->Event("DestroyCanvas", &LyShineExamplesCppExampleBus::Events::DestroyCanvas);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PRIVATE MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateBackground()
    {
        // Get the canvas
        UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(m_canvasId);

        // Create an empty element in the canvas
        AZ::Entity* background = canvas->CreateChildElement("Background");

        // Add a transform component and an image component to the background
        CreateComponent(background, LyShine::UiTransform2dComponentUuid);
        CreateComponent(background, LyShine::UiImageComponentUuid);
        // Add a button component to the background to prevent interactions with interactables on the canvases below this canvas
        CreateComponent(background, LyShine::UiButtonComponentUuid);

        // We want the background to stretch to the corners of the canvas
        // So we set the anchors to go all the way to the right and to the bottom
        AZ::EntityId backgroundId = background->GetId();
        EBUS_EVENT_ID(backgroundId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f), false, false);

        // Set the color of the background image to black
        EBUS_EVENT_ID(backgroundId, UiImageBus, SetColor, AZ::Color(0.f, 0.f, 0.f, 1.f));

        // Set the background button's navigation to none
        EBUS_EVENT_ID(backgroundId, UiNavigationBus, SetNavigationMode, UiNavigationInterface::NavigationMode::None);

        // Now let's create a child of the background
        AZ::Entity* foreground = canvas->CreateChildElement("Foreground");

        // Add a transform and an image to the foreground as well
        CreateComponent(foreground, LyShine::UiTransform2dComponentUuid);
        CreateComponent(foreground, LyShine::UiImageComponentUuid);

        // Stretch it to the corners of the background with the anchors, stretch it 90% of background to still a background outline
        AZ::EntityId foregroundId = foreground->GetId();
        EBUS_EVENT_ID(foregroundId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.1f, 0.1f, 0.9f, 0.9f), false, false);

        return foregroundId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::CreateElementsExample(AZ::EntityId foregroundId)
    {
        // Create the elements examples section title
        CreateText("ElementExamples", false, foregroundId, UiTransform2dInterface::Anchors(0.1f, 0.25f, 0.1f, 0.25f), UiTransform2dInterface::Offsets(),
            "Elements examples:", AZ::Color(0.f, 0.f, 0.f, 1.f), IDraw2d::HAlign::Left, IDraw2d::VAlign::Center,
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);

        // Create an example button
        CreateButton("ButtonExample", false, foregroundId, UiTransform2dInterface::Anchors(0.2f, 0.35f, 0.2f, 0.35f), UiTransform2dInterface::Offsets(-100, -25, 100, 25),
            "Button", AZ::Color(0.604f, 0.780f, 0.839f, 1.f), AZ::Color(0.380f, 0.745f, 0.871f, 1.f), AZ::Color(0.055f, 0.675f, 0.886f, 1.f), AZ::Color(0.f, 0.f, 0.f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);

        // Create an example checkbox
        CreateCheckbox("CheckBoxExample", false, foregroundId, UiTransform2dInterface::Anchors(0.5f, 0.35f, 0.5f, 0.35f), UiTransform2dInterface::Offsets(-25, -25, 25, 25),
            "Checkbox", AZ::Color(1.f, 1.f, 1.f, 1.f), AZ::Color(0.718f, 0.733f, 0.741f, 1.f), AZ::Color(0.831f, 0.914f, 0.937f, 1.f), AZ::Color(0.2f, 1.f, 0.2f, 1.f), AZ::Color(0.f, 0.f, 0.f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);

        // Create an example text input
        CreateTextInput("TextInputExample", false, foregroundId, UiTransform2dInterface::Anchors(0.8f, 0.35f, 0.8f, 0.35f), UiTransform2dInterface::Offsets(-100, -25, 100, 25),
            "", "Type here...", AZ::Color(1.f, 1.f, 1.f, 1.f), AZ::Color(0.616f, 0.792f, 0.851f, 1.0f), AZ::Color(0.616f, 0.792f, 0.851f, 1.0f), AZ::Color(0.f, 0.f, 0.f, 1.f), AZ::Color(0.43f, 0.43f, 0.43f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::CreateBehaviorExample(AZ::EntityId foregroundId)
    {
        // Create the behavior example section title
        CreateText("BehaviorExample", false, foregroundId, UiTransform2dInterface::Anchors(0.1f, 0.5f, 0.1f, 0.5f), UiTransform2dInterface::Offsets(),
            "Behavior example:", AZ::Color(0.f, 0.f, 0.f, 1.f), IDraw2d::HAlign::Left, IDraw2d::VAlign::Center, UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);

        // Here we set up a very simple health bar example, all piloted from C++

        // Create the health bar that we will use to display the health
        // We need a background to show how much of the health has been taken off
        AZ::EntityId healthBarBgId = CreateImage("HealthBarBackground", false, foregroundId, UiTransform2dInterface::Anchors(0.5f, 0.65f, 0.5f, 0.65f), UiTransform2dInterface::Offsets(-400, -50, 400, 50),
            "Textures/Basic/Button_Sliced_Normal.sprite", UiImageInterface::ImageType::Sliced, AZ::Color(0.2f, 0.2f, 0.2f, 1.f), UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        // And then the currently remaining health bar
        m_maxHealthBarOffsets = UiTransform2dInterface::Offsets(10, -40, 790, 40);
        m_healthBar = CreateImage("HealthBar", false, healthBarBgId, UiTransform2dInterface::Anchors(0.0f, 0.5f, 0.0f, 0.5f), m_maxHealthBarOffsets,
                "Textures/Basic/Button_Sliced_Normal.sprite", UiImageInterface::ImageType::Sliced, AZ::Color(0.7f, 0.f, 0.f, 1.f), UiTransformInterface::ScaleToDeviceMode::None);
        m_health = 10;

        // Create a damage button to decrease the health
        m_damageButton = CreateButton("DamageButton", false, foregroundId, UiTransform2dInterface::Anchors(0.35f, 0.8f, 0.35f, 0.8f), UiTransform2dInterface::Offsets(-75, -25, 75, 25),
            "Damage", AZ::Color(0.604f, 0.780f, 0.839f, 1.f), AZ::Color(0.380f, 0.745f, 0.871f, 1.f), AZ::Color(0.055f, 0.675f, 0.886f, 1.f), AZ::Color(0.f, 0.f, 0.f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        UiButtonNotificationBus::MultiHandler::BusConnect(m_damageButton);
        // Create a heal button to increase the health
        m_healButton = CreateButton("HealButton", false, foregroundId, UiTransform2dInterface::Anchors(0.65f, 0.8f, 0.65f, 0.8f), UiTransform2dInterface::Offsets(-75, -25, 75, 25),
            "Heal", AZ::Color(0.604f, 0.780f, 0.839f, 1.f), AZ::Color(0.380f, 0.745f, 0.871f, 1.f), AZ::Color(0.055f, 0.675f, 0.886f, 1.f), AZ::Color(0.f, 0.f, 0.f, 1.f),
            UiTransformInterface::ScaleToDeviceMode::UniformScaleToFit);
        UiButtonNotificationBus::MultiHandler::BusConnect(m_healButton);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId)
    {
        entity->Deactivate();
        entity->CreateComponent(componentTypeId);
        entity->Activate();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateButton(const char* name, bool atRoot, AZ::EntityId parent,
        UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
        const char* text, AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor, AZ::Color textColor,
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode)
    {
        // Create the button element
        AZ::Entity* button = nullptr;
        if (atRoot)
        {
            EBUS_EVENT_ID_RESULT(button, parent, UiCanvasBus, CreateChildElement, name);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(button, parent, UiElementBus, CreateChildElement, name);
        }

        AZ::EntityId buttonId = button->GetId();

        // Set up the button element
        {
            CreateComponent(button, LyShine::UiTransform2dComponentUuid);
            CreateComponent(button, LyShine::UiImageComponentUuid);
            CreateComponent(button, LyShine::UiButtonComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(buttonId), "Transform2d component missing");

            EBUS_EVENT_ID(buttonId, UiTransformBus, SetScaleToDeviceMode, scaleToDeviceMode);
            EBUS_EVENT_ID(buttonId, UiTransform2dBus, SetAnchors, anchors, false, false);
            EBUS_EVENT_ID(buttonId, UiTransform2dBus, SetOffsets, offsets);
            EBUS_EVENT_ID(buttonId, UiImageBus, SetColor, baseColor);

            EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StateHover, buttonId, selectedColor);
            EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StateHover, buttonId, selectedColor.GetA());
            EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StatePressed, buttonId, pressedColor);
            EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StatePressed, buttonId, pressedColor.GetA());

            EBUS_EVENT_ID(buttonId, UiImageBus, SetSpritePathname, "UI/Textures/Prefab/button_normal.sprite");
            EBUS_EVENT_ID(buttonId, UiImageBus, SetImageType, UiImageInterface::ImageType::Sliced);

            EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateSpritePathname, UiInteractableStatesInterface::StateDisabled, buttonId, "UI/Textures/Prefab/button_disabled.sprite");
        }

        AZ::Entity* textElem = nullptr;
        EBUS_EVENT_ID_RESULT(textElem, buttonId, UiElementBus, CreateChildElement, "ButtonText");
        AZ::EntityId textId = textElem->GetId();

        // Create and set up the text element (text displayed on the button)
        {
            CreateComponent(textElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(textElem, LyShine::UiTextComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(textId), "Transform component missing");

            EBUS_EVENT_ID(textId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.5, 0.5, 0.5, 0.5), false, false);
            EBUS_EVENT_ID(textId, UiTransform2dBus, SetOffsets, UiTransform2dInterface::Offsets(0, 0, 0, 0));

            EBUS_EVENT_ID(textId, UiTextBus, SetText, text);
            EBUS_EVENT_ID(textId, UiTextBus, SetTextAlignment, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);
            EBUS_EVENT_ID(textId, UiTextBus, SetColor, textColor);
            EBUS_EVENT_ID(textId, UiTextBus, SetFontSize, 24.0f);
        }

        // Trigger all InGamePostActivate
        EBUS_EVENT_ID(buttonId, UiInitializationBus, InGamePostActivate);
        EBUS_EVENT_ID(textId, UiInitializationBus, InGamePostActivate);

        return buttonId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateCheckbox(const char* name, bool atRoot, AZ::EntityId parent,
        UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets, [[maybe_unused]] const char* text,
        AZ::Color baseColor, AZ::Color selectedColor, [[maybe_unused]] AZ::Color pressedColor, AZ::Color checkColor, [[maybe_unused]] AZ::Color textColor,
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode)
    {
        // Create the checkbox element
        AZ::Entity* checkbox = nullptr;
        if (atRoot)
        {
            EBUS_EVENT_ID_RESULT(checkbox, parent, UiCanvasBus, CreateChildElement, name);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(checkbox, parent, UiElementBus, CreateChildElement, name);
        }

        AZ::EntityId checkboxId = checkbox->GetId();

        // Set up the checkbox element
        {
            CreateComponent(checkbox, LyShine::UiTransform2dComponentUuid);
            CreateComponent(checkbox, LyShine::UiImageComponentUuid);
            CreateComponent(checkbox, LyShine::UiCheckboxComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(checkboxId), "Transform2d component missing");

            EBUS_EVENT_ID(checkboxId, UiTransformBus, SetScaleToDeviceMode, scaleToDeviceMode);
            EBUS_EVENT_ID(checkboxId, UiTransform2dBus, SetAnchors, anchors, false, false);
            EBUS_EVENT_ID(checkboxId, UiTransform2dBus, SetOffsets, offsets);
            EBUS_EVENT_ID(checkboxId, UiImageBus, SetColor, baseColor);

            EBUS_EVENT_ID(checkboxId, UiImageBus, SetSpritePathname, "UI/Textures/Prefab/checkbox_box_normal.sprite");

            EBUS_EVENT_ID(checkboxId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StateHover, checkboxId, selectedColor);
            EBUS_EVENT_ID(checkboxId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StateHover, checkboxId, selectedColor.GetA());
            EBUS_EVENT_ID(checkboxId, UiInteractableStatesBus, SetStateSpritePathname, UiInteractableStatesInterface::StateHover, checkboxId, "UI/Textures/Prefab/checkbox_box_hover.sprite");

            EBUS_EVENT_ID(checkboxId, UiInteractableStatesBus, SetStateSpritePathname, UiInteractableStatesInterface::StateDisabled, checkboxId, "UI/Textures/Prefab/checkbox_box_disabled.sprite");
        }

        // Create the On element (the checkmark that will be displayed when the checkbox is "on")
        AZ::Entity* onElement;
        EBUS_EVENT_ID_RESULT(onElement, checkboxId, UiElementBus, CreateChildElement, "onElem");

        AZ::EntityId onId = onElement->GetId();

        // Set up the On element
        {
            CreateComponent(onElement, LyShine::UiTransform2dComponentUuid);
            CreateComponent(onElement, LyShine::UiImageComponentUuid);

            EBUS_EVENT_ID(onId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.5f, 0.5f, 0.5f, 0.5f), false, false);
            EBUS_EVENT_ID(onId, UiTransform2dBus, SetOffsets, offsets);

            EBUS_EVENT_ID(onId, UiImageBus, SetSpritePathname, "UI/Textures/Prefab/checkbox_check.sprite");
            EBUS_EVENT_ID(onId, UiImageBus, SetColor, checkColor);
        }

        // Link the on and off child entities to the parent checkbox entity.
        EBUS_EVENT_ID(checkboxId, UiCheckboxBus, SetCheckedEntity, onId);

        // Trigger all InGamePostActivate
        EBUS_EVENT_ID(onId, UiInitializationBus, InGamePostActivate);
        EBUS_EVENT_ID(checkboxId, UiInitializationBus, InGamePostActivate);

        return checkboxId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateText(const char* name, bool atRoot, AZ::EntityId parent,
        UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
        const char* text, AZ::Color textColor, IDraw2d::HAlign hAlign, IDraw2d::VAlign vAlign,
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode)
    {
        // Create the text element
        AZ::Entity* textElem = nullptr;
        if (atRoot)
        {
            EBUS_EVENT_ID_RESULT(textElem, parent, UiCanvasBus, CreateChildElement, name);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(textElem, parent, UiElementBus, CreateChildElement, name);
        }

        AZ::EntityId textId = textElem->GetId();

        // Set up the text element
        {
            CreateComponent(textElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(textElem, LyShine::UiTextComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(textId), "Transform component missing");

            EBUS_EVENT_ID(textId, UiTransformBus, SetScaleToDeviceMode, scaleToDeviceMode);
            EBUS_EVENT_ID(textId, UiTransform2dBus, SetAnchors, anchors, false, false);
            EBUS_EVENT_ID(textId, UiTransform2dBus, SetOffsets, offsets);

            EBUS_EVENT_ID(textId, UiTextBus, SetText, text);
            EBUS_EVENT_ID(textId, UiTextBus, SetTextAlignment, hAlign, vAlign);
            EBUS_EVENT_ID(textId, UiTextBus, SetColor, textColor);
        }

        // Trigger all InGamePostActivate
        EBUS_EVENT_ID(textId, UiInitializationBus, InGamePostActivate);

        return textId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateTextInput(const char* name, bool atRoot, AZ::EntityId parent,
        UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
        const char* text, const char* placeHolderText,
        AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor,
        AZ::Color textColor, AZ::Color placeHolderColor,
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode)
    {
        // Create the text input element
        AZ::Entity* textInputElem = nullptr;
        if (atRoot)
        {
            EBUS_EVENT_ID_RESULT(textInputElem, parent, UiCanvasBus, CreateChildElement, name);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(textInputElem, parent, UiElementBus, CreateChildElement, name);
        }

        AZ::EntityId textInputId = textInputElem->GetId();

        // Set up the text input element
        {
            CreateComponent(textInputElem, LyShine::UiTransform2dComponentUuid);
            CreateComponent(textInputElem, LyShine::UiImageComponentUuid);
            CreateComponent(textInputElem, LyShine::UiTextInputComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(textInputId), "Transform2d component missing");

            EBUS_EVENT_ID(textInputId, UiTransformBus, SetScaleToDeviceMode, scaleToDeviceMode);
            EBUS_EVENT_ID(textInputId, UiTransform2dBus, SetAnchors, anchors, false, false);
            EBUS_EVENT_ID(textInputId, UiTransform2dBus, SetOffsets, offsets);
            EBUS_EVENT_ID(textInputId, UiImageBus, SetColor, baseColor);

            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StateHover, textInputId, selectedColor);
            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StateHover, textInputId, selectedColor.GetA());

            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StatePressed, textInputId, pressedColor);
            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StatePressed, textInputId, pressedColor.GetA());

            EBUS_EVENT_ID(textInputId, UiImageBus, SetSpritePathname, "UI/Textures/Prefab/textinput_normal.sprite");
            EBUS_EVENT_ID(textInputId, UiImageBus, SetImageType, UiImageInterface::ImageType::Sliced);

            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateSpritePathname, UiInteractableStatesInterface::StateHover, textInputId, "UI/Textures/Prefab/textinput_hover.sprite");

            EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateSpritePathname, UiInteractableStatesInterface::StateDisabled, textInputId, "UI/Textures/Prefab/textinput_disabled.sprite");
        }

        // Create the text element (what the user will type)
        AZ::EntityId textElemId = CreateText("Text", false, textInputId,
                UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f),
                UiTransform2dInterface::Offsets(5.0f, 5.0f, -5.0f, -5.00f),
                text, textColor, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);

        // reduce the font size
        EBUS_EVENT_ID(textElemId, UiTextBus, SetFontSize, 24.0f);

        // now link the textInputComponent to the child text entity
        EBUS_EVENT_ID(textInputId, UiTextInputBus, SetTextEntity, textElemId);

        // Create the placeholder text element (what appears before any text is typed)
        AZ::EntityId placeHolderElemId = CreateText("PlaceholderText", false, textInputId,
                UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f),
                UiTransform2dInterface::Offsets(5.0f, 5.0f, -5.0f, -5.00f),
                placeHolderText, placeHolderColor, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);

        // reduce the font size
        EBUS_EVENT_ID(placeHolderElemId, UiTextBus, SetFontSize, 24.0f);

        // now link the textInputComponent to the child placeholder text entity
        EBUS_EVENT_ID(textInputId, UiTextInputBus, SetPlaceHolderTextEntity, placeHolderElemId);

        // Trigger all InGamePostActivate
        EBUS_EVENT_ID(textInputId, UiInitializationBus, InGamePostActivate);
        EBUS_EVENT_ID(textElemId, UiInitializationBus, InGamePostActivate);
        EBUS_EVENT_ID(placeHolderElemId, UiInitializationBus, InGamePostActivate);

        return textInputId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EntityId LyShineExamplesCppExample::CreateImage(const char* name, bool atRoot, AZ::EntityId parent,
        UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
        AZStd::string spritePath, UiImageInterface::ImageType imageType, AZ::Color color,
        UiTransformInterface::ScaleToDeviceMode scaleToDeviceMode)
    {
        // Create the image element
        AZ::Entity* image = nullptr;
        if (atRoot)
        {
            EBUS_EVENT_ID_RESULT(image, parent, UiCanvasBus, CreateChildElement, name);
        }
        else
        {
            EBUS_EVENT_ID_RESULT(image, parent, UiElementBus, CreateChildElement, name);
        }

        AZ::EntityId imageId = image->GetId();

        // Set up the image element
        {
            CreateComponent(image, LyShine::UiTransform2dComponentUuid);
            CreateComponent(image, LyShine::UiImageComponentUuid);

            AZ_Assert(UiTransform2dBus::FindFirstHandler(imageId), "Transform2d component missing");

            EBUS_EVENT_ID(imageId, UiTransformBus, SetScaleToDeviceMode, scaleToDeviceMode);
            EBUS_EVENT_ID(imageId, UiTransform2dBus, SetAnchors, anchors, false, false);
            EBUS_EVENT_ID(imageId, UiTransform2dBus, SetOffsets, offsets);

            EBUS_EVENT_ID(imageId, UiImageBus, SetColor, color);
            EBUS_EVENT_ID(imageId, UiImageBus, SetSpritePathname, spritePath);
            EBUS_EVENT_ID(imageId, UiImageBus, SetImageType, imageType);
        }

        // Trigger all InGamePostActivate
        EBUS_EVENT_ID(imageId, UiInitializationBus, InGamePostActivate);

        return imageId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineExamplesCppExample::UpdateHealth(int change)
    {
        // Max health is 10, min health is 0
        m_health = AZStd::max(0, AZStd::min(10, m_health + change));

        // Update the health bar accordingly
        UiTransform2dInterface::Offsets newOffsets = m_maxHealthBarOffsets;
        float healthFraction = m_health / 10.f;
        newOffsets.m_right = m_maxHealthBarOffsets.m_left + (m_maxHealthBarOffsets.m_right - m_maxHealthBarOffsets.m_left) * healthFraction;
        EBUS_EVENT_ID(m_healthBar, UiTransform2dBus, SetOffsets, newOffsets);
    }
}
