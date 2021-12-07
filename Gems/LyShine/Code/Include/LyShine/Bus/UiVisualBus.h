/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <AzCore/Math/Color.h>
#include <IFont.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiVisualInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiVisualInterface() {}

    //! Reset the overrides, used when setting interactable states
    virtual void ResetOverrides() = 0;

    //! Set the override color, used for interactable states
    virtual void SetOverrideColor(const AZ::Color& color) = 0;

    //! Set the override alpha, used for interactable states
    virtual void SetOverrideAlpha(float alpha) = 0;

    //! Set the override sprite, if this visual component uses a sprite this will override it
    //! \param sprite   If null the sprite on the visual component will not be overridden
    //! \param cellIndex This value will be ignored if the given sprite isn't a sprite-sheet spritetype.
    virtual void SetOverrideSprite([[maybe_unused]] ISprite* sprite, [[maybe_unused]] AZ::u32 cellIndex) {};

    //! Set the override font, if this visual component uses a font this will override it
    //! \param font   If null the font on the visual component will not be overridden
    virtual void SetOverrideFont(FontFamilyPtr fontFamily) {};

    //! Set the override font effect, if this visual component uses a font this will override it
    virtual void SetOverrideFontEffect([[maybe_unused]] unsigned int fontEffectIndex) {};

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiVisualInterface> UiVisualBus;
