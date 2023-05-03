/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Configuration object that contains data to setup a Scene.
    struct SceneConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(SceneConfiguration, "{4ABF9993-8E52-4E41-B38D-28FD569B4EAF}");
        static void Reflect(AZ::ReflectContext* context);

        static SceneConfiguration CreateDefault();

        AZStd::string m_sceneName; //!< Name given to the scene.

        AZ::Aabb m_worldBounds = AZ::Aabb::CreateFromMinMax(-AZ::Vector3(1000.f, 1000.f, 1000.f), AZ::Vector3(1000.f, 1000.f, 1000.f));
        AZ::Vector3 m_gravity = AzPhysics::DefaultGravity;
        void* m_customUserData = nullptr;
        bool m_enableCcd = false; //!< Enables continuous collision detection in the world.
        AZ::u32 m_maxCcdPasses = 1; //!< Maximum number of continuous collision detection passes.
        bool m_enableCcdResweep = true; //!< Use a more accurate but more expensive continuous collision detection method.

        //! Enables reporting of changed Simulated bodies on the OnSceneActiveSimulatedBodiesEvent event.
        bool m_enableActiveActors = true; 
        bool m_enablePcm = true; //!< Enables the persistent contact manifold algorithm to be used as the narrow phase algorithm.
        bool m_kinematicFiltering = true; //!< Enables filtering between kinematic/kinematic  objects.
        bool m_kinematicStaticFiltering = true; //!< Enables filtering between kinematic/static objects.
        float m_bounceThresholdVelocity = 2.0f; //!< Relative velocity below which colliding objects will not bounce.

        bool operator==(const SceneConfiguration& other) const;
        bool operator!=(const SceneConfiguration& other) const;

    private:
        AZ::Crc32 GetCcdVisibility() const;
    };

    //! Alias for a list of SceneConfiguration objects, used for the creation of multiple Scenes at once.
    using SceneConfigurationList = AZStd::vector<SceneConfiguration>;
}
