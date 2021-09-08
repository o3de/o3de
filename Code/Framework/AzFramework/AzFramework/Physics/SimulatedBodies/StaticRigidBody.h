/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class ShapeConfiguration;
    class ColliderConfiguration;
    class Shape;
}

namespace AzPhysics
{
    //! Static rigid body.
    struct StaticRigidBody
        : public SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::StaticRigidBody, "{13A677BB-7085-4EDB-BCC8-306548238692}", AzPhysics::SimulatedBody);
        static void Reflect(AZ::ReflectContext* context);

        //! Add a shape to the static rigid body.
        //! @param shape A shared pointer of the shape to add.
        virtual void AddShape(const AZStd::shared_ptr<Physics::Shape>& shape) = 0;

        //! Returns the number of shapes that make up this static rigid body.
        //! @return Returns the number of shapes as a AZ::u32.
        virtual AZ::u32 GetShapeCount() { return 0; }

        //! Returns a shared pointer to the requested shape index.
        //! @param index The index of the shapes to return. Expected to be between 0 and GetShapeCount().
        //! @return Returns a shared pointer of the shape requested or nullptr if index is out of bounds.
        virtual AZStd::shared_ptr<Physics::Shape> GetShape([[maybe_unused]]AZ::u32 index) { return nullptr; }
    };
}
