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
#include <AzFramework/Input/User/LocalUserId.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasManagerInterface
    : public AZ::EBusTraits
{
public: // types

    typedef std::vector<AZ::EntityId> CanvasEntityList;

public:
    //! Create a canvas
    virtual AZ::EntityId CreateCanvas() = 0;

    //! Load a canvas
    virtual AZ::EntityId LoadCanvas(const AZStd::string& canvasPathname) = 0;

    //! Unload a canvas
    virtual void UnloadCanvas(AZ::EntityId canvasEntityId) = 0;

    //! Find a canvas by path, optionally load the canvas if it was not found
    virtual AZ::EntityId FindLoadedCanvasByPathName(const AZStd::string& canvasPathname, bool loadIfNotFound = false) = 0;

    //! Get a list of canvases that are loaded in game, this is sorted by draw order
    virtual CanvasEntityList GetLoadedCanvases() = 0;

    //! Set the local user id that will be used to filter incoming input events for all canvases.
    //! Can be overriden for an individual canvas using UiCanvasInterface::SetLocalUserIdInputFilter.
    virtual void SetLocalUserIdInputFilterForAllCanvases(AzFramework::LocalUserId localUserId) = 0;
};
typedef AZ::EBus<UiCanvasManagerInterface> UiCanvasManagerBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of canvas manager changes
class UiCanvasManagerNotification
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    //////////////////////////////////////////////////////////////////////////

    virtual ~UiCanvasManagerNotification() {}

    //! Called when a canvas has been loaded
    virtual void OnCanvasLoaded([[maybe_unused]] AZ::EntityId canvasEntityId) {}

    //! Called when a canvas has been unloaded/destroyed
    virtual void OnCanvasUnloaded([[maybe_unused]] AZ::EntityId canvasEntityId) {}

    //! Called when a canvas has been reloaded (due to hot-loading)
    //! For a hot-load, the OnCanvasLoaded/OnCanvasUnloaded notifications are not sent - only this one is
    virtual void OnCanvasReloaded([[maybe_unused]] AZ::EntityId canvasEntityId) {}
};

typedef AZ::EBus<UiCanvasManagerNotification> UiCanvasManagerNotificationBus;

