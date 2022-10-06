/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzCore/std/limits.h>
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
        bool m_simulating = false;

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

        //! Helper functions for setting frame ID.
        //! @param frameId Optionally set frame ID for the systems moving the actors back in time.
        void SetFrameId(uint32_t frameId)
        {
            m_frameId = frameId;
        }

        //! Helper functions for getting the set frame ID.
        //! @return Will return the frame ID.
        uint32_t GetFrameId() const
        {
            return m_frameId;
        }

        static constexpr uint32_t UndefinedFrameId = AZStd::numeric_limits<uint32_t>::max();

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
        
        //! Helper to send SyncTransform event to the relevant handlers.
        //! @param deltaTime Frame time.
        void SyncTransform(float deltaTime) const;

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

        //! Helper to register a handler for a SyncTransform event on this Simulated body.
        void RegisterOnSyncTransformHandler(SimulatedBodyEvents::OnSyncTransform::Handler& handler);

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
        SimulatedBodyEvents::OnSyncTransform m_syncTransformEvent;

        void* m_customUserData = nullptr;
        uint32_t m_frameId = UndefinedFrameId;

        // helpers for reflecting to behavior context
        SimulatedBodyEvents::OnCollisionBegin* GetOnCollisionBeginEvent();
        SimulatedBodyEvents::OnCollisionPersist* GetOnCollisionPersistEvent();
        SimulatedBodyEvents::OnCollisionEnd* GetOnCollisionEndEvent();
        SimulatedBodyEvents::OnTriggerEnter* GetOnTriggerEnterEvent();
        SimulatedBodyEvents::OnTriggerExit* GetOnTriggerExitEvent();
        SimulatedBodyEvents::OnSyncTransform* GetOnSyncTransformEvent();
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

    inline void SimulatedBody::RegisterOnSyncTransformHandler(SimulatedBodyEvents::OnSyncTransform::Handler& handler)
    {
        handler.Connect(m_syncTransformEvent);
    }
}
