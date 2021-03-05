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

#include <AzCore/Math/Vector3.h>

namespace Physics
{
    class WorldBody;
    class Shape;

    /// Trigger event raised when an object enters/exits a trigger shape.
    struct TriggerEvent
    {
        Physics::WorldBody* m_triggerBody; ///< The trigger body 
        Physics::Shape* m_triggerShape; ///< The trigger shape
        Physics::WorldBody* m_otherBody; ///< The other body that entered the trigger
        Physics::Shape* m_otherShape; ///< The other shape that entered the trigger
    };

    /// Stores information about the contacts between two overlapping shapes.
    struct Contact
    {
        AZ_CLASS_ALLOCATOR(Contact, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Contact, "{D7439508-ED10-4395-9D48-1FC3D7815361}");

        AZ::Vector3 m_position; ///< The position of the contact
        AZ::Vector3 m_normal; ///< The normal of the contact
        AZ::Vector3 m_impulse; ///< The impulse force applied to separate the bodies
        AZ::u32 m_internalFaceIndex01; ///< Intenal face index of the first shape
        AZ::u32 m_internalFaceIndex02; ///< Internal face index of the second shape
        float m_separation; ///< The separation
    };

    /// A collision event raised when two objects, neither of which can be triggers, overlap.
    struct CollisionEvent
    {
        AZ_CLASS_ALLOCATOR(CollisionEvent, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CollisionEvent, "{7602AA36-792C-4BDC-BDF8-AA16792151A3}");

        Physics::WorldBody* m_body1; ///< The first body
        Physics::Shape* m_shape1; ///< The shape on the first body
        Physics::WorldBody* m_body2; ///< The second body
        Physics::Shape* m_shape2; ///< The shape on the second body
        AZStd::vector<Contact> m_contacts; ///< The contacts between the two shapes
    };

    /// Implement this interface and call SetEventHandler on Physics::World
    /// to receive events from that world.
    /// CActionGame is the default handler for the default physics world which
    /// translates these events into bus events.
    class WorldEventHandler
    {
    public:
        /// Raised when an object starts overlapping with a trigger shape.
        virtual void OnTriggerEnter(const TriggerEvent& triggerEvent) = 0;

        /// Raised when an object stops overlapping with a trigger shape.
        virtual void OnTriggerExit(const TriggerEvent& triggerEvent) = 0;

        /// Raised when two shapes come into contact with each other.
        virtual void OnCollisionBegin(const CollisionEvent& collisionEvent) = 0;

        /// Raised when two shapes continue contact with each other.
        virtual void OnCollisionPersist(const CollisionEvent& collisionEvent) = 0;

        /// Raised when two shapes stop contacting each other.
        virtual void OnCollisionEnd(const CollisionEvent& collisionEvent) = 0;
    };

} //namespace Physics
