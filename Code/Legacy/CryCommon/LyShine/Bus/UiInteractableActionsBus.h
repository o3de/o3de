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
// This bus allows the get/set of properties for a group of actions that many interactable components
// implement.
// It is separate from UiInteractableBus because UiInteractableBus is part of a core system for how
// the UI canvas communincates with any UI element that wants user input. Sometimes UI components
// want input because they are part of a 2D puzzle for example but they do not always want to have
// to support the standard action changes.
class UiInteractableActionsInterface
    : public AZ::ComponentBus
{
public: // types

    typedef AZStd::function<void(AZ::EntityId)> OnActionCallback;

public: // member functions

    virtual ~UiInteractableActionsInterface() {}

    //! Get the hover start action name
    virtual const LyShine::ActionName& GetHoverStartActionName() = 0;

    //! Set the hover start action name
    virtual void SetHoverStartActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the hover end action name
    virtual const LyShine::ActionName& GetHoverEndActionName() = 0;

    //! Set the hover end action name
    virtual void SetHoverEndActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the pressed action name
    virtual const LyShine::ActionName& GetPressedActionName() = 0;

    //! Set the pressed action name
    virtual void SetPressedActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the released action name
    virtual const LyShine::ActionName& GetReleasedActionName() = 0;

    //! Set the released action name
    virtual void SetReleasedActionName(const LyShine::ActionName& actionName) = 0;

    //! Get the hover start callback
    virtual OnActionCallback GetHoverStartActionCallback() = 0;

    //! Set the hover start callback
    virtual void SetHoverStartActionCallback(OnActionCallback onActionCallback) = 0;

    //! Get the hover end callback
    virtual OnActionCallback GetHoverEndActionCallback() = 0;

    //! Set the hover end callback
    virtual void SetHoverEndActionCallback(OnActionCallback onActionCallback) = 0;

    //! Get the pressed callback
    virtual OnActionCallback GetPressedActionCallback() = 0;

    //! Set the pressed callback
    virtual void SetPressedActionCallback(OnActionCallback onActionCallback) = 0;

    //! Get the release callback
    virtual OnActionCallback GetReleasedActionCallback() = 0;

    //! Set the release callback
    virtual void SetReleasedActionCallback(OnActionCallback onActionCallback) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiInteractableActionsInterface> UiInteractableActionsBus;
