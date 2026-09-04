/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework
{
    //! Single-provider hook for sequence authoring behavior that must run at exact points in the Editor lifecycle.
    //!
    //! This is intentionally an AZ::Interface instead of an Editor notification bus.
    //! The callbacks mutate sequence state and must be delivered to at most one active provider.
    class IEditorSequenceSystemLifecycle
    {
    public:
        AZ_TYPE_INFO(IEditorSequenceSystemLifecycle, "{64E299F9-91BE-4FDD-B397-E8AAF5C7E4FF}");

        virtual ~IEditorSequenceSystemLifecycle() = default;

        static IEditorSequenceSystemLifecycle* Get()
        {
            return AZ::Interface<IEditorSequenceSystemLifecycle>::Get();
        }

        //! Called immediately after the Editor environment is installed.
        virtual void OnEditorGameEngineInitialized(bool simulationMode) = 0;

        //! Called after IEditor attaches its game engine and loads Editor templates.
        virtual void OnEditorGameEngineAttached() = 0;

        //! Called before the Editor destroys CrySystem.
        virtual void OnEditorGameEngineShutdown() = 0;

        //! Called on Editor frames before CrySystem pre-update and the component TickBus.
        virtual void OnEditorUpdate() = 0;

        //! Called after the general begin-scene-open notification and before level loading.
        virtual void OnBeginSceneOpen() = 0;

        //! Called after the general begin-game-mode notification and before the in-game flag is set.
        virtual void OnBeginGameMode() = 0;

        //! Called after the in-game flag is set and before PIE entities start.
        virtual void OnGameModeStarted() = 0;

        //! Called after PIE entities stop and before viewport and camera restoration.
        virtual void OnGameModeStopped() = 0;

        //! Called before the in-game flag is cleared and before the general end-game-mode notification.
        virtual void OnEndGameMode(bool simulationMode) = 0;

        //! Called before the general simulation-mode notification and before the simulation flag changes.
        virtual void OnSimulationModeChanged(bool enabled) = 0;
    };

    using EditorSequenceSystemLifecycleInterface = AZ::Interface<IEditorSequenceSystemLifecycle>;
} // namespace AzToolsFramework
