/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_RTTI(StaticRigidBody, "{13A677BB-7085-4EDB-BCC8-306548238692}", SimulatedBody);
        static void Reflect(AZ::ReflectContext* context);

        //Legacy API - may change with LYN-438
        virtual void AddShape(const AZStd::shared_ptr<Physics::Shape>& shape) = 0;
        virtual AZ::u32 GetShapeCount() { return 0; }
        virtual AZStd::shared_ptr<Physics::Shape> GetShape([[maybe_unused]]AZ::u32 index) { return nullptr; }
    };
}
