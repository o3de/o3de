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
class UiScrollBarInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollBarInterface() {}

    //! Get the size of the handle relative to the scrollbar (0 - 1)
    virtual float GetHandleSize() = 0;

    //! Set the size of the handle relative to the scrollbar (0 - 1)
    virtual void SetHandleSize(float size) = 0;

    //! Get the minimum size of the handle in pixels
    virtual float GetMinHandlePixelSize() = 0;

    //! Set the minimum size of the handle in pixels
    virtual void SetMinHandlePixelSize(float size) = 0;

    //! Get the handle entity
    virtual AZ::EntityId GetHandleEntity() = 0;

    //! Set the handle entity
    virtual void SetHandleEntity(AZ::EntityId entityId) = 0;

    //! Get auto fade
    virtual bool IsAutoFadeEnabled() = 0;

    //! Set auto fade
    virtual void SetAutoFadeEnabled(bool isAutoFadeEnabled) = 0;

    //! Get the delay in seconds before the scrollbar will fade if unused
    virtual float GetAutoFadeDelay() = 0;

    //! Set the fade delay in seconds
    virtual void SetAutoFadeDelay(float delay) = 0;

    //! Get the speed in seconds that it will take for the scrollbar to fade to transparent
    virtual float GetAutoFadeSpeed() = 0;

    //! Set the fade speed in seconds
    virtual void SetAutoFadeSpeed(float speed) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiScrollBarInterface> UiScrollBarBus;
