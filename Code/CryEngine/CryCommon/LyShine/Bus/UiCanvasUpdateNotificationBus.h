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
