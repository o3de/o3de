/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX/Configuration/PhysXConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    namespace PhysXInternal
    {
        AzPhysics::CollisionConfiguration CreateDefaultCollisionConfiguration()
        {
            AzPhysics::CollisionConfiguration configuration;
            configuration.m_collisionLayers.SetName(AzPhysics::CollisionLayer::Default, "Default");

            configuration.m_collisionGroups.CreateGroup("All", AzPhysics::CollisionGroup::All, AzPhysics::CollisionGroups::Id(), true);
            configuration.m_collisionGroups.CreateGroup("None", AzPhysics::CollisionGroup::None, AzPhysics::CollisionGroups::Id::Create(), true);

            return configuration;
        }

        bool PhysXSystemConfigurationConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            if (dataElement.GetVersion() <= 1)
            {
                dataElement.RemoveElementByName(AZ_CRC_CE("DefaultMaterialLibrary"));
                AZ_Warning("PhysXSystemConfigurationConverter", false,
                    "Old version of PhysX Configuration data found. 'DefaultMaterialLibrary' element removed."); 
            }

            return true;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(WindConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(LimitsConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(PhysXSystemConfiguration, AZ::SystemAllocator);

    /*static*/ void WindConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysX::WindConfiguration>()
                ->Version(1)
                ->Field("GlobalWindTag", &WindConfiguration::m_globalWindTag)
                ->Field("LocalWindTag", &WindConfiguration::m_localWindTag)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<PhysX::WindConfiguration>("Wind Configuration", "Wind force entity tags.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_globalWindTag,
                        "Global wind tag",
                        "Global wind provider tags.\n"
                        "Global winds apply to entire world.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_localWindTag,
                        "Local wind tag",
                        "Local wind provider tags.\n"
                        "Local winds are constrained to a PhysX collider's boundaries.")
                    ;
            }
        }
    }

    bool WindConfiguration::operator==(const WindConfiguration& other) const
    {
        return m_globalWindTag == other.m_globalWindTag &&
            m_localWindTag == other.m_localWindTag
            ;
    }

    bool WindConfiguration::operator!=(const WindConfiguration& other) const
    {
        return !(*this == other);
    }

    /*static*/ void LimitsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysX::LimitsConfiguration>()
                ->Version(1)
                ->Field("SanityBounds", &LimitsConfiguration::m_sanityBoundsHalfExtents)
                ->Field("MaxActors", &LimitsConfiguration::m_maxActors)
                ->Field("MaxDynamicBodies", &LimitsConfiguration::m_maxDynamicBodies)
                ->Field("MaxStaticShapes", &LimitsConfiguration::m_maxStaticShapes)
                ->Field("MaxDynamicShapes", &LimitsConfiguration::m_maxDynamicShapes)
                ->Field("MaxAggregates", &LimitsConfiguration::m_maxAggregates)
                ->Field("MaxConstraints", &LimitsConfiguration::m_maxConstraintShaders)
                ->Field("MaxBroadPhaseRegions", &LimitsConfiguration::m_maxBroadphaseRegions)
                ->Field("MaxBroadPhaseOverlaps", &LimitsConfiguration::m_maxBroadphaseOverlaps)
                ->Field("ScratchBufferSize", &LimitsConfiguration::m_numScratchBufferBlocks)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<PhysX::LimitsConfiguration>("Limits Configuration", "Expected simulation soft limits.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_sanityBoundsHalfExtents, "Sanity Bounds Half-Extents",
                        "These bounds are used to check the position values of rigid actors inserted into the scene, and positions set for rigid actors already within the scene.\n"
                        "PhysX will report if a body is outside of these bounds.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxActors, "Max Actors",
                        "This includes Articulation Links and derived physics body types.\n"
                        "A reasonable value might be 10240. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxDynamicBodies, "Max Dynamic Bodies",
                        "A reasonable value might be 256-512. Note, all bodies are actors. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxStaticShapes, "Max Static Shapes",
                        "A reasonable value might be 2048-4096 for a more densily populated static world.\n"
                        "Note, not all actors/bodies have shapes. Bodies can have compound shapes. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxDynamicShapes, "Max Dynamic Shapes",
                        "This should be slightly higher than your max dynamic bodies. Note, not all actors/bodies have shapes. Bodies can have compound shapes. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxAggregates, "Max Aggregates",
                        "This is cluster of bodies which act/move together, such as a Ragdoll. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxConstraintShaders, "Max Constraints",
                        "This can vary significantly depending on game or simulation requirements. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxBroadphaseRegions, "Max Broadphase Regions",
                        "Broadphases are expensive to compute, and the limit should be kept low. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_maxBroadphaseOverlaps, "Max Broadphase Overlaps",
                        "Number of overlapping bodies colliding in broadphase.\n"
                        "A reasonable value might be 1024, but will need to be adjusted depending on game/simulation requirements. 0 indicates no limit.")
                        ->Attribute(AZ::Edit::Attributes::Max, UINT_MAX)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LimitsConfiguration::m_numScratchBufferBlocks, "Scratch Buffer Blocks",
                        "Number of 16K memory blocks to size the temporary scratch buffer used during physics simulation.\n"
                        "Default is 2048 (16K * 2048 = 32MB)")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ;
            }
        }
    }

    bool LimitsConfiguration::operator==(const LimitsConfiguration& other) const
    {
        return m_sanityBoundsHalfExtents == other.m_sanityBoundsHalfExtents &&
            m_maxActors == other.m_maxActors &&
            m_maxDynamicBodies == other.m_maxDynamicBodies &&
            m_maxStaticShapes == other.m_maxStaticShapes &&
            m_maxDynamicShapes == other.m_maxDynamicShapes &&
            m_maxAggregates == other.m_maxAggregates &&
            m_maxConstraintShaders == other.m_maxConstraintShaders &&
            m_maxBroadphaseRegions == other.m_maxBroadphaseRegions &&
            m_maxBroadphaseOverlaps == other.m_maxBroadphaseOverlaps && 
            m_numScratchBufferBlocks == other.m_numScratchBufferBlocks
            ;
    }

    bool LimitsConfiguration::operator!=(const LimitsConfiguration& other) const
    {
        return !(*this == other);
    }

    /*static*/ void PhysXSystemConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AzPhysics::SystemConfiguration::Reflect(context);
        WindConfiguration::Reflect(context);
        LimitsConfiguration::Reflect(context);

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::PhysXSystemConfiguration, AzPhysics::SystemConfiguration>()
                ->Version(3, &PhysXInternal::PhysXSystemConfigurationConverter)
                ->Field("WindConfiguration", &PhysXSystemConfiguration::m_windConfiguration)
                ->Field("LimitsConfiguration", &PhysXSystemConfiguration::m_limitsConfiguration)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                // this is needed so the edit context of AzPhysics::SystemConfiguration can be used.
                editContext->Class<PhysX::PhysXSystemConfiguration>("System Configuration", "PhysX system configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    /*static*/ PhysXSystemConfiguration PhysXSystemConfiguration::CreateDefault()
    {
        PhysXSystemConfiguration systemConfig;
        systemConfig.m_collisionConfig = PhysXInternal::CreateDefaultCollisionConfiguration();
        return systemConfig;
    }

    bool PhysXSystemConfiguration::operator==(const PhysXSystemConfiguration& other) const
    {
        return AzPhysics::SystemConfiguration::operator==(other) &&
            m_windConfiguration == other.m_windConfiguration &&
            m_limitsConfiguration == other.m_limitsConfiguration
            ;
    }

    bool PhysXSystemConfiguration::operator!=(const PhysXSystemConfiguration& other) const
    {
        return !(*this == other);
    }
}
