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
//! UiCanvasRefBus
//! Allows a reference to a UI Canvas entity (which is loaded from a .uicanvas asset file) to 
//! associated with a component entity in the level.
//! This is used for convenience by flow graph nodes and also for supprting rendering UI
//! canvases in the 3D world on a component entity.
class UiCanvasRefInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasRefInterface() {}

    //! Get the UI canvas associated with this entity
    virtual AZ::EntityId GetCanvas() = 0;
};

using UiCanvasRefBus = AZ::EBus<UiCanvasRefInterface>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasRefNotificationBus
//! Clients can connect to this bus to receive notifications of when the canvas reference
//! in a UiCanvasRef changes to a different canvas
class UiCanvasRefNotifications
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasRefNotifications() {}

    //! Called when the canvas referenced by a canvas ref component changes
    //! This can happen through a load, unload or set
    virtual void OnCanvasRefChanged([[maybe_unused]] AZ::EntityId uiCanvasRefEntity, [[maybe_unused]] AZ::EntityId uiCanvasEntity) {}
};

using UiCanvasRefNotificationBus = AZ::EBus<UiCanvasRefNotifications>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasAssetRefBus
//! Allows loading and unloading of a UI canvas asset using a pathname stored in a component on
//! an entity in the level.
class UiCanvasAssetRefInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasAssetRefInterface() {}

    //! Get the canvas pathname. This is the pathname of the canvas that this component
    //! will load (either on activate or when told to load by the LoadCanvas method)
    //! The Canvas pathname can be empty and the associated canvas can be set via SetCanvas
    virtual AZStd::string GetCanvasPathname() = 0;

    //! Set the canvas pathname
    virtual void SetCanvasPathname(const AZStd::string& pathname) = 0;

    //! Get the flag indicating whether this component will automatically load the canvas
    virtual bool GetIsAutoLoad() = 0;

    //! Set the flag indicating whether this component will automatically load the canvas
    virtual void SetIsAutoLoad(bool isAutoLoad) = 0;

    //! Get the flag indicating whether the canvas should be loaded in a disabled state
    virtual bool GetShouldLoadDisabled() = 0;

    //! Set the flag indicating whether the canvas should be loaded in a disabled state
    virtual void SetShouldLoadDisabled(bool shouldLoadDisabled) = 0;

    //! Load the UI canvas using the stored asset ref
    virtual AZ::EntityId LoadCanvas() = 0;

    //! Unload the UI canvas using the stored asset ref (if it is owned by this component)
    virtual void UnloadCanvas() = 0;
};

using UiCanvasAssetRefBus = AZ::EBus<UiCanvasAssetRefInterface>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasAssetRefNotificationBus
//! Clients can connect to this bus to receive notifications of when a canvas is loaded into
//! a canvas ref component on an entity
class UiCanvasAssetRefNotifications
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasAssetRefNotifications() {}

    //! Called when the canvas ref loads a UI canvas
    virtual void OnCanvasLoadedIntoEntity([[maybe_unused]] AZ::EntityId uiCanvasEntity) {}
};

using UiCanvasAssetRefNotificationBus = AZ::EBus<UiCanvasAssetRefNotifications>;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasProxyRefBus
//! Allows an entity in a level to share a reference to a UI canvas that was loaded into another
//! UiCanvasRef.
//! This bus is only needed to allow two entities in the world to use the same instance of a
//! UI canvas asset.
class UiCanvasProxyRefInterface
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasProxyRefInterface() {}

    //! Set the entity that is managing the UI canvas for this proxy
    //! This will cause the OnCanvasRefChanged event to be sent to any UiCanvasRefNotifications
    virtual void SetCanvasRefEntity(AZ::EntityId canvasAssetRefEntity) = 0;
};

using UiCanvasProxyRefBus = AZ::EBus<UiCanvasProxyRefInterface>;

