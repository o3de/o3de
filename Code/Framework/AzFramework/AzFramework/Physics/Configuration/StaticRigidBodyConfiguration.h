/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>

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
    struct StaticRigidBodyConfiguration : public SimulatedBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(StaticRigidBodyConfiguration, "{E68A14C0-21DC-4FC7-9AD0-04BB9D972004}", SimulatedBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        StaticRigidBodyConfiguration() = default;
        virtual ~StaticRigidBodyConfiguration() = default;

        //! Variant to support multiple having the system creating the Shape(s) or just providing the Shape(s) that have been created externally.
        //! See ShapeVariantData for more information.
        ShapeVariantData m_colliderAndShapeData;
    };
}
