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
#include <AzCore/Math/Color.h>

class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
// This bus allows the get/set of properties for a group of states that many interactable components
// implement.
// It is separate from UiInteractableBus because UiInteractableBus is part of a core system for how
// the UI canvas communincates with any UI element that wants user input. Sometimes UI components
// want input because they are part of a 2D puzzle for example but they do not always want to have
// to support the standard state changes.
class UiInteractableStatesInterface
    : public AZ::ComponentBus
{
public: // types

    //! The different visual states that an interactable can be in. An enum class is avoided so that 
    //! derived components of UiInteractableComponent can extend with additional states.
    using State = int;
    enum
    {
        StateNormal = 0,
        StateHover,
        StatePressed,
        StateDisabled,
        NumStates
    };

public: // member functions

    virtual ~UiInteractableStatesInterface() {}

    //! Set the color to be used for the given target when the interactable is in the given state
    //! If the interactable already has a color action for this state/target combination then replaces it
    virtual void SetStateColor(State state, AZ::EntityId target, const AZ::Color& color) = 0;

    //! Get the color to be used for the given target when the interactable is in the given state
    //! \return the color to be used for the given target when the interactable is in the given state
    virtual AZ::Color GetStateColor(State state, AZ::EntityId target) = 0;

    //! Get whether the interactable has a color action for this state/target combination
    //! \return true if the interactable has a color action for this state/target combination
    virtual bool HasStateColor(State state, AZ::EntityId target) = 0;

    //! Set the alpha to be used for the given target when the interactable is in the given state
    //! If the interactable already has an alpha action for this state/target combination then replaces it
    virtual void SetStateAlpha(State state, AZ::EntityId target, float alpha) = 0;

    //! Get the alpha to be used for the given target when the interactable is in the given state
    //! \return the alpha to be used for the given target when the interactable is in the given state
    virtual float GetStateAlpha(State state, AZ::EntityId target) = 0;

    //! Get whether the interactable has an alpha action for this state/target combination
    //! \return true if the interactable has an alpha action for this state/target combination
    virtual bool HasStateAlpha(State state, AZ::EntityId target) = 0;

    //! Set the sprite to be used for the given target when the interactable is in the given state
    //! If the interactable already has a sprite action for this state/target combination then replaces it
    virtual void SetStateSprite(State state, AZ::EntityId target, ISprite* sprite) = 0;

    //! Get the sprite to be used for the given target when the interactable is in the given state
    //! \return the sprite to be used for the given target when the interactable is in the given state
    virtual ISprite* GetStateSprite(State state, AZ::EntityId target) = 0;

    //! Set the sprite path to be used for the given target when the interactable is in the given state
    //! If the interactable already has a sprite action for this state/target combination then replaces it
    virtual void SetStateSpritePathname(State state, AZ::EntityId target, const AZStd::string& spritePath) = 0;

    //! Get the sprite path to be used for the given target when the interactable is in the given state
    //! \return the sprite path to be used for the given target when the interactable is in the given state
    virtual AZStd::string GetStateSpritePathname(State state, AZ::EntityId target) = 0;

    //! Get whether the interactable has a sprite action for this state/target combination
    //! \return true if the interactable has a sprite action for this state/target combination
    virtual bool HasStateSprite(State state, AZ::EntityId target) = 0;

    //! Set the font to be used for the given target when the interactable is in the given state
    //! If the interactable already has a font action for this state/target combination then replaces it
    virtual void SetStateFont(State state, AZ::EntityId target, const AZStd::string& fontPathname, unsigned int fontEffectIndex) = 0;

    //! Get the font path to be used for the given target when the interactable is in the given state
    //! \return the font path to be used for the given target when the interactable is in the given state
    virtual AZStd::string GetStateFontPathname(State state, AZ::EntityId target) = 0;

    //! Get the font effect to be used for the given target when the interactable is in the given state
    //! \return the font effect to be used for the given target when the interactable is in the given state
    virtual unsigned int GetStateFontEffectIndex(State state, AZ::EntityId target) = 0;

    //! Get whether the interactable has a font action for this state/target combination
    //! \return true if the interactable has a font action for this state/target combination
    virtual bool HasStateFont(State state, AZ::EntityId target) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiInteractableStatesInterface> UiInteractableStatesBus;

