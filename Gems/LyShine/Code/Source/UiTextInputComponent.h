/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiTextInputBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiInteractableComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/TextureAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextInputComponent
    : public UiInteractableComponent
    , public UiInitializationBus::Handler
    , public UiTextInputBus::Handler
{
public: //types

    using EntityComboBoxVec = AZStd::vector < AZStd::pair<AZ::EntityId, AZStd::string> >;

public: // member functions

    AZ_COMPONENT(UiTextInputComponent, LyShine::UiTextInputComponentUuid, AZ::Component);

    UiTextInputComponent();
    ~UiTextInputComponent() override;

    // UiInteractableInterface
    bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;
    bool HandleReleased(AZ::Vector2 point) override;
    bool HandleEnterPressed(bool& shouldStayActive) override;
    bool HandleEnterReleased() override;
    bool HandleAutoActivation() override;
    bool HandleTextInput(const AZStd::string& textUTF8) override;
    bool HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys) override;
    void InputPositionUpdate(AZ::Vector2 point) override;
    void LostActiveStatus() override;
    // ~UiInteractableInterface

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiInitializationInterface
    void InGamePostActivate() override;
    // ~UiInitializationInterface

    // UiTextInputInterface
    bool GetIsPasswordField() override;
    void SetIsPasswordField(bool passwordField) override;
    uint32_t GetReplacementCharacter() override;
    void SetReplacementCharacter(uint32_t replacementChar) override;

    AZ::Color GetTextSelectionColor() override;
    void SetTextSelectionColor(const AZ::Color& color) override;
    AZ::Color GetTextCursorColor() override;
    void SetTextCursorColor(const AZ::Color& color) override;

    float GetCursorBlinkInterval() override;
    void SetCursorBlinkInterval(float interval) override;
    int GetMaxStringLength() override;
    void SetMaxStringLength(int maxCharacters) override;

    TextInputCallback GetOnChangeCallback() override;
    void SetOnChangeCallback(TextInputCallback callbackFunction) override;
    TextInputCallback GetOnEndEditCallback() override;
    void SetOnEndEditCallback(TextInputCallback callbackFunction) override;
    TextInputCallback GetOnEnterCallback() override;
    void SetOnEnterCallback(TextInputCallback callbackFunction) override;

    const LyShine::ActionName& GetChangeAction() override;
    void SetChangeAction(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetEndEditAction() override;
    void SetEndEditAction(const LyShine::ActionName& actionName) override;
    const LyShine::ActionName& GetEnterAction() override;
    void SetEnterAction(const LyShine::ActionName& actionName) override;

    AZ::EntityId GetTextEntity() override;
    void SetTextEntity(AZ::EntityId textEntity) override;
    AZStd::string GetText() override;
    void SetText(const AZStd::string& text) override;
    AZ::EntityId GetPlaceHolderTextEntity() override;
    void SetPlaceHolderTextEntity(AZ::EntityId textEntity) override;

    bool GetIsClipboardEnabled() override;
    void SetIsClipboardEnabled(bool enableClipboard) override;

    // ~UiTextInputInterface

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    // UiInteractableComponent
    bool IsAutoActivationSupported() override;
    // ~UiInteractableComponent

    void BeginEditState();
    void EndEditState();
    float GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint);
    void CheckForDragOrHandOffToParent(AZ::Vector2 point);

    //! Changes the DisplayedTextFunction callback of our child m_textEntity
    //! If m_isPasswordField is true, we assign a callback that replaces the contents
    //! of the displayed string with our m_replacementCharacter, otherwise we assign a
    //! null callback (default behavior).
    void UpdateDisplayedTextFunction();

    void OnReplacementCharacterChange();

    EntityComboBoxVec PopulateTextEntityList();

    UiInteractableStatesInterface::State ComputeInteractableState() override;

protected:  // static member functions

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
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiTextInputComponent);

    //! Change text and notify listeners
    void ChangeText(const AZStd::string& textString);

    //! Make cursor visible when a change in text or cursor position has occurred
    void ResetCursorBlink();

    uint GetMultiByteCharLength(const AZStd::string& textString);

    void CheckStartTextInput();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    bool m_isDragging;
    bool m_isEditing;
    bool m_isTextInputStarted;

    int m_textCursorPos;            //!< UTF8 element/character index into rendered string.

    int m_textSelectionStartPos;    //!< UTF8 index that will different in value from m_textCursorPos
                                    //!< if a range of text is currently selected.

    float m_cursorBlinkStartTime;

    // We use EntityIds for the text and placeholder entities so the the reflection
    // system can save and load the references.
    AZ::EntityId m_textEntity;
    AZ::EntityId m_placeHolderTextEntity;

    AZ::Color m_textSelectionColor;
    AZ::Color m_textCursorColor;
    int m_maxStringLength;
    float m_cursorBlinkInterval;

    bool m_childTextStateDirtyFlag;

    TextInputCallback m_onChange;
    TextInputCallback m_onEndEdit;
    TextInputCallback m_onEnter;

    LyShine::ActionName m_changeAction;
    LyShine::ActionName m_endEditAction;
    LyShine::ActionName m_enterAction;

    uint32_t m_replacementCharacter;    //!< If this component is configured as a password field (m_isPasswordField),
                                        //!< then we'll use this UTF8 character to replace the contents of the m_textEntity
                                        //!< string when we render (note that the string contents of m_textEntity
                                        //!< remain unaltered and this only affects rendering).

    bool m_isPasswordField;             //!< True if m_textEntity should be treated as a password field, false otherwise

    bool m_clipInputText;               //!< True if input text should be visually clipped to child text element, false otherwise.

    bool m_enableClipboard;             //!< True if copy/cut/paste should be supported, false otherwise.
};
