/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/SimpleAsset.h>

#include <Blast/BlastActorData.h>
#include <Blast/BlastDebug.h>

namespace Blast
{
    class BlastActor;

    //! Bus that handles damage being dealt to the actors in the  family.
    //! For convenience family connects on its own id as well as all entities that represent blast actors.
    //! If the damage is applied on family id then it's applied to all appropriate actors (which might be limited to
    //! just that actor, e. g. in StressDamage). If the damage is applied on actor id then it's only applied to that
    //! actor.
    class BlastFamilyDamageRequests : public AZ::ComponentBus
    {
    public:
        virtual AZ::EntityId GetFamilyId() = 0;

        //! Radial damage function.
        //! @param position The global position of the damage's hit.
        //! @param damage How much damage to deal.
        //! @param minRadius Damages all chunks/bonds that are in the range [0, minRadius] with full damage.
        //! @param maxRadius Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly
        //! decreasing damage.
        virtual void RadialDamage(const AZ::Vector3& position, float minRadius, float maxRadius, float damage) = 0;

        //! Capsule radial damage function.
        //! @param position0 The global position of one of the capsule's ends.
        //! @param position1 The global position of another of the capsule's ends.
        //! @param damage How much damage to deal.
        //! @param minRadius Damages all chunks/bonds that are in the range [0, minRadius] with full damage.
        //! @param maxRadius Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly
        //! decreasing damage.
        virtual void CapsuleDamage(
            const AZ::Vector3& position0, const AZ::Vector3& position1, float minRadius, float maxRadius,
            float damage) = 0;

        //! Shear damage function.
        //! @param position The global position of the damage's hit.
        //! @param normal The normal of the damage's hit.
        //! @param damage How much damage to deal.
        //! @param minRadius Damages all chunks/bonds that are in the range [0, minRadius] with full damage.
        //! @param maxRadius Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly
        //! decreasing damage.
        virtual void ShearDamage(
            const AZ::Vector3& position, const AZ::Vector3& normal, float minRadius, float maxRadius, float damage) = 0;

        //! Triangle damage function.
        //! @param position0, position1, position2  Vertices of the triangle.
        //! @param damage How much damage to deal.
        virtual void TriangleDamage(
            const AZ::Vector3& position0, const AZ::Vector3& position1, const AZ::Vector3& position2, float damage) = 0;

        //! Impact spread damage function. Differs from radial damage by calculating distance between nodes in support
        //! graph using Breadth First Search over the bonds of the graph instead of Euclidean distance.
        //! @param position The global position of the damage's hit.
        //! @param damage How much damage to deal.
        //! @param minRadius Damages all chunks/bonds that are in the range [0, minRadius] with full damage.
        //! @param maxRadius Damages all chunks/bonds that are in the range [minRadius, maxRadius] with linearly
        //! decreasing damage.
        virtual void ImpactSpreadDamage(
            const AZ::Vector3& position, float minRadius, float maxRadius, float damage) = 0;

        //! Stress damage function that only accepts a position.
        //! @param position The global position of the damage's hit.
        //! @param force The force applied at the position.
        virtual void StressDamage(const AZ::Vector3& position, const AZ::Vector3& force) = 0;

        //! Stress damage function that only accepts a position.
        //! @param blastActor Actor to apply damage on.
        //! @param position The global position of the damage's hit.
        //! @param force The impulse applied at the position.
        virtual void StressDamage(const BlastActor& blastActor, const AZ::Vector3& position, const AZ::Vector3& force) = 0;

        //! Destroys actor. This is not similar to damage, because the actor is not split into child actors, but rather
        //! just removed from simulation. This method still triggers notifications. Calling this on a family id will
        //! destroy all of its actors.
        virtual void DestroyActor() = 0;
    };
    using BlastFamilyDamageRequestBus = AZ::EBus<BlastFamilyDamageRequests>;

    //! Bus that handles non-damage requests to the family.
    //! Only listens on it's own id.
    class BlastFamilyComponentRequests : public AZ::ComponentBus
    {
    public:
        //! Get a list of all actors of this family
        //! The pointers in the vector are guaranteed to be valid.
        virtual AZStd::vector<const BlastActor*> GetActors() = 0;

        //! Same as GetActors but for script canvas exposure.
        virtual AZStd::vector<BlastActorData> GetActorsData() = 0;

        //! Fill debug render buffer with debug visualization data based on debug mode.
        virtual void FillDebugRenderBuffer(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode debugRenderMode) = 0;

        //! Apply accumulated stress damage onto the actors in the family.
        //! Should only be invoked by BlastSystemComponent.
        virtual void ApplyStressDamage() = 0;

        //! Sync positions of meshes of chunks with corresponding actors.
        //! Should only be invoked by BlastSystemComponent.
        virtual void SyncMeshes() = 0;
    };
    using BlastFamilyComponentRequestBus = AZ::EBus<BlastFamilyComponentRequests>;

    //! Notifications about actor creations/destructions emitted by the family.
    class BlastFamilyComponentNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Called when BlastFamily creates a new actor.
        //! @param actor The newly created actor.
        virtual void OnActorCreated(const BlastActor& actor) = 0;

        //! Called before a BlastFamily destroys an actor.
        //! @param actor The actor to be destroyed.
        virtual void OnActorDestroyed(const BlastActor& actor) = 0;
    };
    using BlastFamilyComponentNotificationBus = AZ::EBus<BlastFamilyComponentNotifications>;
} // namespace Blast
