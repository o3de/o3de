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

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UiDropdownOptionComponent is a component that is designed to work in conjunction with the
//! UiDropdownComponent. It represents any option of that dropdown that the user should be able to
//! select to update the value of the dropdown.
class UiDropdownOptionInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDropdownOptionInterface() {}

    //! Get the owning dropdown of this option
    virtual AZ::EntityId GetOwningDropdown() = 0;

    //! Set the owning dropdown of this option
    virtual void SetOwningDropdown(AZ::EntityId owningDropdown) = 0;

    //! Get the text element of this option
    virtual AZ::EntityId GetTextElement() = 0;

    //! Set the text element of this option
    virtual void SetTextElement(AZ::EntityId textElement) = 0;

    //! Get the icon element of this option
    virtual AZ::EntityId GetIconElement() = 0;

    //! Set the icon element of this option
    virtual void SetIconElement(AZ::EntityId iconElement) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDropdownOptionInterface> UiDropdownOptionBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropdownOptionNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDropdownOptionNotifications() {}

    //! Notify listeners that the dropdown option was selected
    virtual void OnDropdownOptionSelected() {}
};

typedef AZ::EBus<UiDropdownOptionNotifications> UiDropdownOptionNotificationBus;