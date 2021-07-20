/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextInputInterface
    : public AZ::ComponentBus
{
public: // types

    typedef AZStd::function<void(AZ::EntityId, const AZStd::string&)> TextInputCallback;

public: // member functions

    virtual ~UiTextInputInterface() {}

    //! Get the color to be used for the text background when it is selected
    virtual AZ::Color GetTextSelectionColor() = 0;

    //! Set the color to be used for the text background when it is selected
    virtual void SetTextSelectionColor(const AZ::Color& color) = 0;

    //! Get the color to be used for the text cursor
    virtual AZ::Color GetTextCursorColor() = 0;

    //! Set the color to be used for the text cursor
    virtual void SetTextCursorColor(const AZ::Color& color) = 0;

    //! Get the cursor blink interval
    virtual float GetCursorBlinkInterval() = 0;

    //! Set the cursor blink interval, 0 means no blink
    virtual void SetCursorBlinkInterval(float interval) = 0;

    //! Get the maximum number of character allowed in the edited string
    virtual int GetMaxStringLength() = 0;

    //! Set the maximum number of character allowed in the edited string
    //! \param maxCharacters, a value of 0 means none allowed, -1 means no limit
    virtual void SetMaxStringLength(int maxCharacters) = 0;

    //! Get the on-change callback (called when a character added/removed/changed)
    virtual TextInputCallback GetOnChangeCallback() = 0;

    //! Set the on-change callback (called when a character added/removed/changed)
    virtual void SetOnChangeCallback(TextInputCallback callbackFunction) = 0;

    //! Get the on-end-edit callback (called when edit of text is completed)
    virtual TextInputCallback GetOnEndEditCallback() = 0;

    //! Set the on-end-edit callback (called when edit of text is completed)
    virtual void SetOnEndEditCallback(TextInputCallback callbackFunction) = 0;

    //! Get the on-enter callback (called when Enter is pressed on keyboard)
    virtual TextInputCallback GetOnEnterCallback() = 0;

    //! Set the on-enter callback (called when Enter is pressed on keyboard)
    virtual void SetOnEnterCallback(TextInputCallback callbackFunction) = 0;

    //! Get the "change" action name, the action is sent to canvas listeners when text is changed
    virtual const LyShine::ActionName& GetChangeAction() = 0;

    //! Set the "change" action name
    virtual void SetChangeAction(const LyShine::ActionName& actionName) = 0;

    //! Get the "end edit" action name, the action is sent to canvas listeners when the
    //! editing of the text is finished - i.e. when the text input component is no longer active
    virtual const LyShine::ActionName& GetEndEditAction() = 0;

    //! Set the "end edit" action name
    virtual void SetEndEditAction(const LyShine::ActionName& actionName) = 0;

    //! Get the "enter" action name, the action is sent to canvas listeners when enter is pressed
    virtual const LyShine::ActionName& GetEnterAction() = 0;

    //! Set the "enter" action name
    virtual void SetEnterAction(const LyShine::ActionName& actionName) = 0;

    //! Get the entity id for the text element being edited by this component
    virtual AZ::EntityId GetTextEntity() = 0;

    //! Set the entity id for the text element being edited by this component
    //! This must be a child of this entity
    virtual void SetTextEntity(AZ::EntityId textEntity) = 0;

    //! Get the text string being edited by this component (from the text element)
    virtual AZStd::string GetText() = 0;

    virtual void SetText(const AZStd::string& text) = 0;

    //! Get the entity id for the placeholder text element for this component
    virtual AZ::EntityId GetPlaceHolderTextEntity() = 0;

    //! Set the entity id for the placeholder text element for this component
    //! This must be a child of this entity
    virtual void SetPlaceHolderTextEntity(AZ::EntityId textEntity) = 0;

    //! True if this text input is configured as a password field, false otherwise
    //!
    //! Password fields will render the displayed text with all of the characters
    //! of the input string replaced with a character.
    virtual bool GetIsPasswordField() = 0;

    //! Allows this text input to be configured as a password field.
    virtual void SetIsPasswordField(bool passwordField) = 0;

    //! Returns the UTF8 character to be used to display password fields
    //
    //! Note that having a replacement character configured doesn't determine
    //! whether this input is configured as a password field (see GetIsPasswordField).
    virtual uint32_t GetReplacementCharacter() = 0;

    //! Sets the UTF8 character that should be used for displaying text in password fields.
    //
    //! Note that setting a replacement character doesn't determine whether this
    //! text input will be used as a password field (see GetIsPasswordField).
    virtual void SetReplacementCharacter(uint32_t replacementChar) = 0;

    //! True if copy/cut/paste should be supported, false otherwise
    virtual bool GetIsClipboardEnabled() = 0;

    //! Allows copy/cut/paste support for this text input
    virtual void SetIsClipboardEnabled(bool enableClipboard) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiTextInputInterface> UiTextInputBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTextInputNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiTextInputNotifications() {}

    //! Called when a character added/removed/change
    virtual void OnTextInputChange([[maybe_unused]] const AZStd::string& textString) {}

    //! Called when edit of text is completed
    virtual void OnTextInputEndEdit([[maybe_unused]] const AZStd::string& textString) {}

    //! Called when Enter is pressed on keyboard
    virtual void OnTextInputEnter([[maybe_unused]] const AZStd::string& textString) {}
};

typedef AZ::EBus<UiTextInputNotifications> UiTextInputNotificationBus;
