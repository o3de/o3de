/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>

namespace Blast
{
    //! Class that describes configuration of the rigid bodies used in Blast Actors.
    class BlastActorConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastActorConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BlastActorConfiguration, "{949E731B-0418-4B70-8969-2871F66CF463}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastActorConfiguration>()
                    ->Version(1)
                    ->Field("CollisionLayer", &BlastActorConfiguration::m_collisionLayer)
                    ->Field("CollisionGroupId", &BlastActorConfiguration::m_collisionGroupId)
                    ->Field("Simulated", &BlastActorConfiguration::m_isSimulated)
                    ->Field("InSceneQueries", &BlastActorConfiguration::m_isInSceneQueries)
                    ->Field("CcdEnabled", &BlastActorConfiguration::m_isCcdEnabled)
                    ->Field("ColliderTag", &BlastActorConfiguration::m_tag);

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<BlastActorConfiguration>("BlastActorConfiguration", "Configuration for a collider")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isSimulated, "Simulated",
                            "If set, this actor's collider will partake in collision in the physical simulation")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isInSceneQueries, "In Scene Queries",
                            "If set, this actor's collider will be visible for scene queries")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isCcdEnabled, "CCD Enabled",
                            "If set, actor's rigid body will have CCD enabled")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_collisionLayer, "Collision Layer",
                            "The collision layer assigned to the collider")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_collisionGroupId, "Collides With",
                            "The collision group containing the layers this collider collides with")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_tag, "Tag",
                            "Tag used to identify colliders from one another");
                }
            }
        }

        BlastActorConfiguration() = default;
        BlastActorConfiguration(const BlastActorConfiguration&) = default;
        virtual ~BlastActorConfiguration() = default;

        AzPhysics::CollisionLayer m_collisionLayer; //! Which collision layer is this actor's collider on.
        AzPhysics::CollisionGroups::Id m_collisionGroupId; //! Which layers does this actor's collider collide with.
        bool m_isSimulated = true; //! Should this actor's shapes partake in collision in the physical simulation.
        bool m_isInSceneQueries =
            true; //! Should this actor's shapes partake in scene queries (ray casts, overlap tests, sweeps).
        bool m_isCcdEnabled = true; //! Should this actor's rigid body be using CCD.
        AZStd::string m_tag; //! Identification tag for the collider
    };
} // namespace Blast
