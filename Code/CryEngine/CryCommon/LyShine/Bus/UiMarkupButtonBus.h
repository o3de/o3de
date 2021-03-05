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
#include <AzCore/Math/Color.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! A markup button allows for button-like behavior on markup text.
//!
//! The markup itself is contained within a text component. The markup button
//! handles interaction with the text, and can also apply styling (such as
//! coloring for button/clickable text).
//!
//! Markup button behavior is only intended for mouse interactions.
class UiMarkupButtonInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiMarkupButtonInterface() {}

    //! Get the link color
    virtual AZ::Color GetLinkColor() = 0;

    //! Set the link color
    virtual void SetLinkColor(const AZ::Color& linkColor) = 0;

    //! Get the link hover color
    virtual AZ::Color GetLinkHoverColor() = 0;

    //! Set the link hover color
    virtual void SetLinkHoverColor(const AZ::Color& linkHoverColor) = 0;

public: // static member data

        //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiMarkupButtonInterface> UiMarkupButtonBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiMarkupButtonNotifications
    : public AZ::ComponentBus
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions

    virtual ~UiMarkupButtonNotifications() {}

    virtual void OnHoverStart(int id, const AZStd::string& action, const AZStd::string& data) = 0;
    virtual void OnHoverEnd(int id, const AZStd::string& action, const AZStd::string& data) = 0;
    virtual void OnPressed(int id, const AZStd::string& action, const AZStd::string& data) = 0;
    virtual void OnReleased(int id, const AZStd::string& action, const AZStd::string& data) = 0;
    virtual void OnClick(int id, const AZStd::string& action, const AZStd::string& data) = 0;
};

typedef AZ::EBus<UiMarkupButtonNotifications> UiMarkupButtonNotificationsBus;

