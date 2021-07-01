/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>

namespace EMotionFX
{
    class Actor;

    /**
     * @brief An Actor pointer that unregisters itself when it goes out of
     * scope
     *
     * This class allows for simple functionality of automatically registering
     * and unregistering an actor from the manager. Its primary use case is the
     * ActorAsset, that shares ownership with the manager. But it can also be
     * used anywhere that needs to make an Actor that needs to be in the
     * Manager for a given period of time. A good example of this is anything
     * that needs Actor commands to work on an actor that is made in a given
     * scope. One main place where this happens is in the Actor asset processor
     * code.
     */
    class AutoRegisteredActor
    {
    public:
        AutoRegisteredActor() = default;

        template<class T>
        AutoRegisteredActor(AZStd::shared_ptr<T> actor)
            : m_actor(AZStd::move(actor))
        {
            Register(m_actor);
        }
        template<class T>
        AutoRegisteredActor(AZStd::unique_ptr<T> actor)
            : m_actor(AZStd::move(actor))
        {
            Register(m_actor);
        }

        // This class is not copyable, because a given actor cannot be
        // registered with the manager multiple times
        AutoRegisteredActor(const AutoRegisteredActor&) = delete;
        AutoRegisteredActor& operator=(const AutoRegisteredActor&) = delete;

        AutoRegisteredActor(AutoRegisteredActor&& other) noexcept
        {
            *this = AZStd::move(other);
        }

        AutoRegisteredActor& operator=(AutoRegisteredActor&& other) noexcept
        {
            if (this != &other)
            {
                Unregister(m_actor);
                m_actor = AZStd::move(other.m_actor);
            }
            return *this;
        }

        ~AutoRegisteredActor()
        {
            Unregister(m_actor);
        }

        Actor* operator->() const
        {
            return m_actor.operator->();
        }

        operator bool() const
        {
            return static_cast<bool>(m_actor);
        }

        Actor* get() const
        {
            return m_actor.get();
        }

    private:
        void Register(const AZStd::shared_ptr<Actor>& actor)
        {
            if (actor)
            {
                GetActorManager().RegisterActor(actor);
            }
        }

        void Unregister(const AZStd::shared_ptr<Actor>& actor)
        {
            if (actor)
            {
                GetActorManager().UnregisterActor(actor);
            }
        }

        AZStd::shared_ptr<Actor> m_actor;
    };
} // namespace EMotionFX
