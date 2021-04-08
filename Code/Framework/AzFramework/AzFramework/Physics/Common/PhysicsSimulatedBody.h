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

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    namespace Automation
    {
        class SimulatedBodyCollisionAutomationHandler;
        class SimulatedBodyTriggerAutomationHandler;
    }
    class Scene;
    struct CollisionEvent;
    struct TriggerEvent;

    //! Base class for all Simulated bodies in Physics.
    struct SimulatedBody
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::SimulatedBody, "{BCC37A4F-1C05-4660-9E41-0CCF2D5E7175}");
        static void Reflect(AZ::ReflectContext* context);

        virtual ~SimulatedBody() = default;

        //! The current Scene the simulated body is contained.
        SceneHandle m_sceneOwner = AzPhysics::InvalidSceneHandle;

        //! The handle to this simulated body
        SimulatedBodyHandle m_bodyHandle = AzPhysics::InvalidSimulatedBodyHandle;

        //! Flag to determine if the body is part of the simulation.
        //! When true the body will be affected by any forces, collisions, and found with scene queries.
        bool m_simulating = true;

        //! Helper functions for setting user data.
        //! @param userData Can be a pointer to any type as internally will be cast to a void*. Object lifetime not managed by the SimulatedBody.
        template<typename T>
        void SetUserData(T* userData);
        //! Helper functions for getting the set user data.
        //! @return Will return a void* to the user data set.
        void* GetUserData()
        {
            return m_customUserData;
        }

        //! Perform a ray cast on this Simulated Body.
        //! @param request The request to make.
        //! @return Returns the closest hit, if any, against this simulated body.
        virtual AzPhysics::SceneQueryHit RayCast(const RayCastRequest& request) = 0;

        //! Helper to direct the CollisionEvent to the correct handler.
        //! Will invoke the OnCollisionBegin or OnCollisionPersist or OnCollisionEnd event.
        //! @param collision The collision data to be routed.
        void ProcessCollisionEvent(const CollisionEvent& collision) const;

        //! Helper to direct the TriggerEvent to the correct handler.
        //! Will invoke the OnTriggerEnter or OnTriggerExitevent event.
        //! @param trigger The trigger data to be routed.
        void ProcessTriggerEvent(const TriggerEvent& trigger) const;

        //! Helpers to register a handler for Collision events on this Simulated body.
        //! OnCollisionBegin is when two bodies start to collide.
        //! OnCollisionPersist is when two bodies continue to collide.
        //! OnCollisionEnd is when two bodies stop colliding.
        void RegisterOnCollisionBeginHandler(SimulatedBodyEvents::OnCollisionBegin::Handler& handler);
        //! see RegisterOnCollisionBeginHandler
        void RegisterOnCollisionPersistHandler(SimulatedBodyEvents::OnCollisionPersist::Handler& handler);
        //! see RegisterOnCollisionBeginHandler
        void RegisterOnCollisionEndHandler(SimulatedBodyEvents::OnCollisionEnd::Handler& handler);

        //! Helpers to register a handler for Trigger Events on this Simulated body.
        //! OnTriggerEnter is when a body enters a trigger.
        //! OnTriggerExit is when a body leaves a trigger.
        void RegisterOnTriggerEnterHandler(SimulatedBodyEvents::OnTriggerEnter::Handler& handler);
        //! see RegisterOnTriggerEnterHandler
        void RegisterOnTriggerExitHandler(SimulatedBodyEvents::OnTriggerExit::Handler& handler);

        virtual AZ::Crc32 GetNativeType() const = 0;
        virtual void* GetNativePointer() const = 0;

        //! Helper to get the scene this body is attached too.
        //! @return Returns a pointer to the scene.
        virtual Scene* GetScene();

        // Temporary until LYN-438 work is complete - from old WorldBody Class
        virtual AZ::EntityId GetEntityId() const = 0;
        virtual AZ::Transform GetTransform() const = 0;
        virtual void SetTransform(const AZ::Transform& transform) = 0;
        virtual AZ::Vector3 GetPosition() const = 0;
        virtual AZ::Quaternion GetOrientation() const = 0;
        virtual AZ::Aabb GetAabb() const = 0;

    private:
        friend class Automation::SimulatedBodyCollisionAutomationHandler;
        friend class Automation::SimulatedBodyTriggerAutomationHandler;

        SimulatedBodyEvents::OnCollisionBegin m_collisionBeginEvent;
        SimulatedBodyEvents::OnCollisionPersist m_collisionPersistEvent;
        SimulatedBodyEvents::OnCollisionEnd m_collisionEndEvent;
        SimulatedBodyEvents::OnTriggerEnter m_triggerEnterEvent;
        SimulatedBodyEvents::OnTriggerExit m_triggerExitEvent;

        void* m_customUserData = nullptr;

        // helpers for reflecting to behavior context
        SimulatedBodyEvents::OnCollisionBegin* GetOnCollisionBeginEvent();
        SimulatedBodyEvents::OnCollisionPersist* GetOnCollisionPersistEvent();
        SimulatedBodyEvents::OnCollisionEnd* GetOnCollisionEndEvent();
        SimulatedBodyEvents::OnTriggerEnter* GetOnTriggerEnterEvent();
        SimulatedBodyEvents::OnTriggerExit* GetOnTriggerExitEvent();
    };
    //! Alias for a list of non owning weak pointers to SimulatedBody objects.
    using SimulatedBodyList = AZStd::vector<SimulatedBody*>;

    template<typename T>
    void SimulatedBody::SetUserData(T* userData)
    {
        m_customUserData = static_cast<void*>(userData);
    }

    inline void SimulatedBody::RegisterOnCollisionBeginHandler(SimulatedBodyEvents::OnCollisionBegin::Handler& handler)
    {
        handler.Connect(m_collisionBeginEvent);
    }

    inline void SimulatedBody::RegisterOnCollisionPersistHandler(SimulatedBodyEvents::OnCollisionPersist::Handler& handler)
    {
        handler.Connect(m_collisionPersistEvent);
    }

    inline void SimulatedBody::RegisterOnCollisionEndHandler(SimulatedBodyEvents::OnCollisionEnd::Handler& handler)
    {
        handler.Connect(m_collisionEndEvent);
    }

    inline void SimulatedBody::RegisterOnTriggerEnterHandler(SimulatedBodyEvents::OnTriggerEnter::Handler& handler)
    {
        handler.Connect(m_triggerEnterEvent);
    }

    inline void SimulatedBody::RegisterOnTriggerExitHandler(SimulatedBodyEvents::OnTriggerExit::Handler& handler)
    {
        handler.Connect(m_triggerExitEvent);
    }
}
