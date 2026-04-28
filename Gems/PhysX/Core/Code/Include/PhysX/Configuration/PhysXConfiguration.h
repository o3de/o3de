/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    //! PhysX wind settings.
    class WindConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO(PhysX::WindConfiguration, "{6EA3E646-ECDA-4044-912D-5722D5100066}");
        static void Reflect(AZ::ReflectContext* context);

        /// Tag value that will be used to identify entities that provide global wind value.
        /// Global wind has no bounds and affects objects across entire level.
        AZStd::string m_globalWindTag = "global_wind";
        /// Tag value that will be used to identify entities that provide local wind value.
        /// Local wind is only applied within bounds defined by PhysX collider.
        AZStd::string m_localWindTag = "wind";

        bool operator==(const WindConfiguration& other) const;
        bool operator!=(const WindConfiguration& other) const;
    };

    //! PhysX world, actor, and collision limitis.
    //! These are soft limits that give the system an initial allocation guess, which can improve performance vs unbounded.
    class LimitsConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO(PhysX::LimitsConfiguration, "{F748DC89-FCEF-4C13-81BD-BAE4CEECE09F}");
        static void Reflect(AZ::ReflectContext* context);

        /// @brief The bounds used to sanity check user-set positions of actors and articulation links.
        /// @details These bounds are used to check the position values of rigid actors inserted into the scene, and positions set for rigid actors already within the scene.
        AZ::Vector3 m_sanityBoundsHalfExtents = AZ::Vector3(1000.f, 1000.f, 1000.f); // This can be reflected to the editor unlike AZ::Aabb
        
        AZ::u32 m_maxActors = 0; //!< Expected max expected actors in a scene. 0 indicates no limit.

        AZ::u32 m_maxDynamicBodies = 0; //!< Expected max dynamic bodies in a scene. Note, all bodies are actors. 0 indicates no limit.

        AZ::u32 m_maxStaticShapes = 0; //!< Expected max static shapes in a scene. 0 indicates no limit.

        AZ::u32 m_maxDynamicShapes = 0; //!< Expected max dynamic shapes in a scene. 0 indicates no limit.

        AZ::u32 m_maxAggregates = 0; //!< Expected max aggregates in a scene. This is cluster of bodies such as a Ragdoll. 0 indicates no limit.

        AZ::u32 m_maxConstraintShaders = 0; //!< Expected max constraint shaders in a scene. 0 indicates no limit.

        AZ::u32 m_maxBroadphaseRegions = 0; //!< Expected max broadphase regions in a scene. 0 indicates no limit.

        AZ::u32 m_maxBroadphaseOverlaps = 0; //!< Expected max broadphase overlaps in a scene. 0 indicates no limit.

        AZ::u32 m_numScratchBufferBlocks = 2048; //!< Number of 16K blocks in scratch buffer for calculation during physics simulation. // TODO: add this to scene simulate call

        bool operator==(const LimitsConfiguration& other) const;
        bool operator!=(const LimitsConfiguration& other) const;
    };

    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct PhysXSystemConfiguration : public AzPhysics::SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(PhysX::PhysXSystemConfiguration, "{6E25A37B-2109-452C-97C9-B737CC72704F}");
        static void Reflect(AZ::ReflectContext* context);

        static PhysXSystemConfiguration CreateDefault();

        WindConfiguration m_windConfiguration; //!< Wind configuration for PhysX.
        LimitsConfiguration m_limitsConfiguration; //!< Limits configuration for PhysX.

        bool operator==(const PhysXSystemConfiguration& other) const;
        bool operator!=(const PhysXSystemConfiguration& other) const;
    };
}
