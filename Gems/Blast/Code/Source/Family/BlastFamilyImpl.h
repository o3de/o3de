/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Asset/BlastAsset.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <Blast/BlastActor.h>
#include <Blast/BlastDebug.h>
#include <Blast/BlastMaterial.h>

#include <Family/ActorTracker.h>
#include <Family/BlastFamily.h>

namespace Blast
{
    //! Blast family implementation that listens to events from the TkFamily and manages BlastActors for a destructible
    //! object.
    class BlastFamilyImpl
        : public BlastFamily
        , public Nv::Blast::TkEventListener
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastFamilyImpl, AZ::SystemAllocator, 0);

        //! Constructor for the BlastPxFamily
        //! @param desc Creation descriptor for the family based on the Blast library's ExtPxFamily.
        BlastFamilyImpl(const BlastFamilyDesc& desc);
        ~BlastFamilyImpl();

        // BlastFamily interface
        bool Spawn(const AZ::Transform& transform) override;
        void Despawn() override;

        void HandleEvents(const Nv::Blast::TkEvent* events, uint32_t eventCount) override;
        void DestroyActor(BlastActor* blastActor) override;

        ActorTracker& GetActorTracker() override;
        const Nv::Blast::TkFamily* GetTkFamily() const override;
        Nv::Blast::TkFamily* GetTkFamily() override;
        const Nv::Blast::ExtPxAsset& GetPxAsset() const override;
        const BlastActorConfiguration& GetActorConfiguration() const override;

        void FillDebugRender(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode, float renderScale) override;

        //! TkEventListener interface.
        void receive(const Nv::Blast::TkEvent* events, uint32_t eventCount) override;

    private:
        void CreateActors(const AZStd::vector<BlastActorDesc>& tkActors);
        void DestroyActors(const AZStd::unordered_set<BlastActor*>& actors);

        void DispatchActorCreated(const BlastActor& actor);
        void DispatchActorDestroyed(const BlastActor& actor);

        AZStd::vector<BlastActorDesc> CalculateActorsDescFromFamily(const AZ::Transform& transform);

        void HandleSplitEvent(
            const Nv::Blast::TkSplitEvent* splitEvent, AZStd::vector<BlastActorDesc>& newActorsDesc,
            AZStd::unordered_set<BlastActor*>& actorsToDelete);

        // Calculates actor description for an actor that has a parent.
        BlastActorDesc CalculateActorDesc(
            AzPhysics::SimulatedBody* parentBody, bool parentStatic, AZ::Transform parentTransform,
            Nv::Blast::TkActor* tkActor);

        // Calculates actor description for an actor that does not have a parent.
        BlastActorDesc CalculateActorDesc(const AZ::Transform& transform, Nv::Blast::TkActor* tkActor);

        void FillDebugRenderHealthGraph(
            DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode, const Nv::Blast::TkActor& actor);
        void FillDebugRenderAccelerator(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode);


        const BlastAsset& m_asset;
        ActorTracker m_actorTracker;
        physx::unique_ptr<Nv::Blast::TkFamily> m_tkFamily;
        AZStd::shared_ptr<BlastActorFactory> m_actorFactory;
        AZStd::shared_ptr<EntityProvider> m_entityProvider;
        BlastListener* m_listener;

        const Physics::MaterialId m_physicsMaterialId;
        const BlastMaterial m_blastMaterial;
        const BlastActorConfiguration& m_actorConfiguration;
        AZ::Transform m_initialTransform;
        bool m_isSpawned;
    };
} // namespace Blast
