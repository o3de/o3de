/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UI Dropdown Component is an interactable component which displays a list of options when clicked.
//! In its default state, the dropdown display a simple button, next to an arrow indicating the dropdown
//! functionality. When the arrow / option is clicked, the dropdown list appears, displaying the options available.
//! If the list is too long to be displayed, a scrollbar can be added to scroll up and down the list of options.
class UiDropdownInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDropdownInterface() {}

    //! Get the currently selected option
    virtual AZ::EntityId GetValue() = 0;

    //! Set the currently selected option manually
    virtual void SetValue(AZ::EntityId value) = 0;

    //! Get the content element this dropdown will expand
    virtual AZ::EntityId GetContent() = 0;

    //! Set the content element this dropdown will expand
    virtual void SetContent(AZ::EntityId content) = 0;

    //! Get whether this dropdown should expand automatically on hover
    virtual bool GetExpandOnHover() = 0;

    //! Set whether this dropdown should expand automatically on hover
    virtual void SetExpandOnHover(bool expandOnHover) = 0;

    //! Get how long to wait before expanding upon hover / collapsing upon exit
    virtual float GetWaitTime() = 0;

    //! Set how long to wait before expanding upon hover / collapsing upon exit
    virtual void SetWaitTime(float waitTime) = 0;

    //! Get whether this dropdown should collapse when the user clicks outside
    virtual bool GetCollapseOnOutsideClick() = 0;

    //! Set whether this dropdown should collapse when the user clicks outside
    virtual void SetCollapseOnOutsideClick(bool collapseOnOutsideClick) = 0;

    //! Get the element the dropdown content will parent to when expanded (the canvas by default)
    virtual AZ::EntityId GetExpandedParentId() = 0;

    //! Set the element the dropdown content will parent to when expanded
    virtual void SetExpandedParentId(AZ::EntityId expandedParentId) = 0;

    //! Get the text element to display to show the currently selected option
    virtual AZ::EntityId GetTextElement() = 0;

    //! Set the text element to display to show the currently selected option
    virtual void SetTextElement(AZ::EntityId textElement) = 0;

    //! Get the icon element to display to show the currently selected option
    virtual AZ::EntityId GetIconElement() = 0;

    //! Set the icon element to display to show the currently selected option
    virtual void SetIconElement(AZ::EntityId iconElement) = 0;

    //! Expand the dropdown
    virtual void Expand() = 0;

    //! Collapse the dropdown
    virtual void Collapse() = 0;

    //! Get the name of the action that is sent when the dropdown is expanded
    virtual const LyShine::ActionName& GetExpandedActionName() = 0;

    //! Set the name of the action that is sent when the dropdown is expanded
    virtual void SetExpandedActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the name of the action that is sent when the dropdown is collapsed
    virtual const LyShine::ActionName& GetCollapsedActionName() = 0;

    //! Set the name of the action that is sent when the dropdown is collapsed
    virtual void SetCollapsedActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the name of the action that is sent when the dropdown value is changed
    virtual const LyShine::ActionName& GetOptionSelectedActionName() = 0;

    //! Set the name of the action that is sent when the dropdown value is changed
    virtual void SetOptionSelectedActionName(const LyShine::ActionName& actionName) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiDropdownInterface> UiDropdownBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropdownNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiDropdownNotifications() {}

    //! Notify listeners that the dropdown was expanded
    virtual void OnDropdownExpanded() {}

    //! Notify listeners that the dropdown was collapsed
    virtual void OnDropdownCollapsed() {}

    //! Notify listeners that an option was selected
    virtual void OnDropdownValueChanged([[maybe_unused]] AZ::EntityId option) {}
};

typedef AZ::EBus<UiDropdownNotifications> UiDropdownNotificationBus;
