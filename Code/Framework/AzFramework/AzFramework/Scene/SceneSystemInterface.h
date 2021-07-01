/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/functional.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>

namespace AzFramework
{
    //! Interface used to create, get, or destroy scenes.
    //! This interface is single thread and is intended to be called from a single thread, commonly the main thread. The exception
    //! is connecting events, which is thread safe.
    class ISceneSystem
    {
    public:
        AZ_RTTI(AzFramework::ISceneSystem, "{DAE482A8-88AE-4BD3-8A5B-52D19A96E15F}");
        AZ_DISABLE_COPY_MOVE(ISceneSystem);

        enum class EventType
        {
            SceneCreated,
            ScenePendingRemoval
        };
        using SceneEvent = AZ::Event<EventType, const AZStd::shared_ptr<Scene>&>;

        ISceneSystem() = default;
        virtual ~ISceneSystem() = default;

        using ActiveIterationCallback = AZStd::function<bool(const AZStd::shared_ptr<Scene>& scene)>;
        using ZombieIterationCallback = AZStd::function<bool(Scene& scene)>;

        //! Creates a scene with a given name.
        //!  - If there is already a scene with the provided name this will return AZ::Failure(). 
        virtual AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> CreateScene(AZStd::string_view name) = 0;

        //! Creates a scene with a given name and a parent.
        //!  - If there is already a scene with the provided name this will return AZ::Failure().
        virtual AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> CreateSceneWithParent(
            AZStd::string_view name, AZStd::shared_ptr<Scene> parent) = 0;

        //! Gets a scene with a given name
        //!  - If a scene does not exist with the given name, nullptr is returned.
        [[nodiscard]] virtual AZStd::shared_ptr<Scene> GetScene(AZStd::string_view name) = 0;

        //! Iterates over all scenes that are in active use. Iteration stops if the callback returns false or all scenes have been listed.
        virtual void IterateActiveScenes(const ActiveIterationCallback& callback) = 0;
        //! Iterates over all zombie scenes. Zombie scenes are scenes that have been removed but still have references held on to. This can
        //! happen because scenes hold on to subsystems that can't immediately be deleted. These subsystems may still require being called
        //! such as a periodic tick. Iteration stops if the callback returns false or all scenes have been listed.
        virtual void IterateZombieScenes(const ZombieIterationCallback& callback) = 0;

        //! Remove a scene with a given name and return if the operation was successful.
        virtual bool RemoveScene(AZStd::string_view name) = 0;

        //! Connects the provided handler to the events that are called after scenes are created or before they get removed.
        virtual void ConnectToEvents(SceneEvent::Handler& handler) = 0;

    protected:
        // Strictly a forwarding function to call private functions on the scene.
        void MarkSceneForDestruction(Scene& scene) { scene.MarkForDestruction(); }
    };

    using SceneSystemInterface = AZ::Interface<ISceneSystem>;

    // EBus wrapper for ScriptCanvas
    class ISceneSystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using ILoggerRequestBus = AZ::EBus<ISceneSystem, ISceneSystemRequests>;
} // AzFramework
