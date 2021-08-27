/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class Shape;   
}

namespace AzPhysics
{
    struct SimulatedBody;

    //! Trigger event raised when an object enters/exits a trigger shape.
    struct TriggerEvent
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(TriggerEvent, "{7A0851A3-2CBD-4A03-85D5-1C40221E7F61}");
        static void Reflect(AZ::ReflectContext* context);

        enum class Type : AZ::u8
        {
            Enter,
            Exit
        };

        Type m_type; //! The type of trigger event.
        SimulatedBodyHandle m_triggerBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the trigger body
        SimulatedBody* m_triggerBody = nullptr; //!< The trigger body 
        Physics::Shape* m_triggerShape = nullptr; //!< The trigger shape
        SimulatedBodyHandle m_otherBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the body that entered the trigger
        SimulatedBody* m_otherBody = nullptr; //!< The other body that entered the trigger
        Physics::Shape* m_otherShape = nullptr; //!< The other shape that entered the trigger

    private:
        // helpers for reflecting to behaviour context
        AZ::EntityId GetTriggerEntityId() const;
        AZ::EntityId GetOtherEntityId() const;
    };
    using TriggerEventList = AZStd::vector<TriggerEvent>;

    //! Stores information about the contacts between two overlapping shapes.
    struct Contact
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(Contact, "{D7439508-ED10-4395-9D48-1FC3D7815361}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector3 m_position; //!< The position of the contact
        AZ::Vector3 m_normal; //!< The normal of the contact
        AZ::Vector3 m_impulse; //!< The impulse force applied to separate the bodies
        AZ::u32 m_internalFaceIndex01 = 0; //!< Internal face index of the first shape
        AZ::u32 m_internalFaceIndex02 = 0; //!< Internal face index of the second shape
        float m_separation = 0.0f; //!< The separation
    };

    //! A collision event raised when two objects, neither of which can be triggers, overlap.
    struct CollisionEvent
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionEvent, "{7602AA36-792C-4BDC-BDF8-AA16792151A3}");
        static void Reflect(AZ::ReflectContext* context);

        enum class Type : AZ::u8
        {
            Begin,
            Persist,
            End
        };
        Type m_type; //! The Type of collision event.
        SimulatedBodyHandle m_bodyHandle1 = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the first body
        SimulatedBody* m_body1 = nullptr; //! The first body
        Physics::Shape* m_shape1 = nullptr; //!< The shape on the first body
        SimulatedBodyHandle m_bodyHandle2 = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the second body
        SimulatedBody* m_body2 = nullptr; //! The second body
        Physics::Shape* m_shape2 = nullptr; //!< The shape on the second body
        AZStd::vector<Contact> m_contacts; //!< The contacts between the two shapes

    private:
        // helpers for reflecting to behaviour context
        AZ::EntityId GetBody1EntityId() const;
        AZ::EntityId GetBody2EntityId() const;
    };
    using CollisionEventList = AZStd::vector<CollisionEvent>;
}
