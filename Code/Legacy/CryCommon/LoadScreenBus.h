/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_LOADSCREENCOMPONENT_ENABLED      (1)

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Component/ComponentBus.h>

class LoadScreenInterface
    : public AZ::ComponentBus
{
public:
    using MutexType = AZStd::recursive_mutex;

    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender() = 0;

    //! Invoked when the game load screen should become visible.
    virtual void GameStart() = 0;

    //! Invoked when the level load screen should become visible.
    virtual void LevelStart() = 0;

    //! Invoked when the load screen should be paused.
    virtual void Pause() = 0;

    //! Invoked when the load screen should be resumed.
    virtual void Resume() = 0;

    //! Invoked when the load screen should be stopped.
    virtual void Stop() = 0;

    //! Invoked to find out if loading screen is playing.
    virtual bool IsPlaying() = 0;
};
using LoadScreenBus = AZ::EBus<LoadScreenInterface>;

//! Interface for notifying load screen providers that specific load events are happening.
//! This is meant to notify systems to connect/disconnect to the LoadScreenUpdateNotificationBus if necessary.
struct LoadScreenNotifications
    : public AZ::EBusTraits
{
    //! Invoked when the game/engine loading starts. Returns true if any provider handles this.
    virtual bool NotifyGameLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when level loading starts. Returns true if any provider handles this.
    virtual bool NotifyLevelLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when loading finishes.
    virtual void NotifyLoadEnd() = 0;
};
using LoadScreenNotificationBus = AZ::EBus<LoadScreenNotifications>;

//! Interface for triggering load screen updates and renders. Has different methods for single threaded vs multi threaded.
//! This is a separate bus from the LoadScreenNotificationBus to avoid threading issues and to allow implementers to conditionally attach
//! from inside LoadScreenNotificationBus::NotifyGameLoadStart/NotifyLevelLoadStart
struct LoadScreenUpdateNotifications
    : public AZ::EBusTraits
{
    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadUpdate(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadRender() = 0;
};
using LoadScreenUpdateNotificationBus = AZ::EBus<LoadScreenUpdateNotifications>;

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
