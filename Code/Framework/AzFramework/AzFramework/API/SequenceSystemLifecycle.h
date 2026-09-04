/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>

namespace AzFramework
{
    //! Single-provider hook for a sequence system that must run at exact points in the host lifecycle.
    //!
    //! This is intentionally an AZ::Interface instead of a notification bus.
    //! Sequence state changes are ordering-sensitive and must be delivered to at most one active provider.
    class ISequenceSystemLifecycle
    {
    public:
        AZ_TYPE_INFO(ISequenceSystemLifecycle, "{5076BDBE-54A1-41E4-9957-C2562009697F}");

        virtual ~ISequenceSystemLifecycle() = default;

        static ISequenceSystemLifecycle* Get()
        {
            return AZ::Interface<ISequenceSystemLifecycle>::Get();
        }

        //! Called after the host registers its CVars and before console-created callbacks and configuration loading.
        virtual void OnSystemCVarRegistry() = 0;

        //! Called once system initialization is complete and before the general CrySystem initialized notification.
        virtual void OnSystemInitialized(bool skipSequenceSystem) = 0;

        //! Called after level teardown and before the general CrySystem shutdown notification.
        virtual void OnSystemShutdown() = 0;

        //! Called at the end of the system pre-update, before the component TickBus runs.
        virtual void OnPreUpdate(bool isEditorUpdate) = 0;

        //! Called at the start of the system post-update, after the component TickBus and before audio updates.
        virtual void OnPostUpdate(bool isEditorUpdate) = 0;

        //! Called after level entities are ready and before level precaching begins.
        virtual void OnLevelEntitiesReady() = 0;

        //! Called at the former sequence cleanup point, before the level-unload-complete notification.
        virtual void OnLevelUnload() = 0;
    };

    using SequenceSystemLifecycleInterface = AZ::Interface<ISequenceSystemLifecycle>;
} // namespace AzFramework
