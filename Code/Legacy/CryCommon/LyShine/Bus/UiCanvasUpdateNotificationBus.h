/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Elements that require update notifications should connect to this bus using the entity id of their
//! containing canvas.
class UiCanvasUpdateNotification
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiCanvasUpdateNotification() {}

    //! Update the component. This is called when the game is running.
    //! It is different from the TickBus in that it is called only when the canvas is updated.
    //! So it is not called if the canvas is disabled.
    virtual void Update(float deltaTime) = 0;

    //! Update the component while in the editor.
    //! This is called every frame when in the editor and the game is NOT running.
    virtual void UpdateInEditor(float /*deltaTime*/) {}

public: // static member data

    //! Multiple components on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
};

typedef AZ::EBus<UiCanvasUpdateNotification> UiCanvasUpdateNotificationBus;
