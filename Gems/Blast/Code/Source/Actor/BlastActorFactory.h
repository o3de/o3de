/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Actor/BlastActorDesc.h>
#include <Blast/BlastActor.h>
#include <PhysX/ColliderComponentBus.h>

namespace Blast
{
    class BlastFamily;

    //! Interface that creates new actors and calculates necessary parts of actor description
    class BlastActorFactory
    {
    public:
        virtual ~BlastActorFactory() = default;

        //! Creates actor based on a description. Caller has to call DestroyActor on the actor afterwards.
        virtual BlastActor* CreateActor(const BlastActorDesc& desc) = 0;

        //! Destroys an existing actor.
        virtual void DestroyActor(BlastActor* actor) = 0;

        //! Calculate chunks that are going to simulate this actor. See more at BlastActorDesc::m_chunkIndices.
        virtual AZStd::vector<uint32_t> CalculateVisibleChunks(
            const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor) const = 0;

        //! Calculate whether an actor represented by a single leaf chunk. See more at BlastActorDesc::m_isLeafChunk.
        virtual bool CalculateIsLeafChunk(
            const Nv::Blast::TkActor& tkActor, const AZStd::vector<uint32_t>& chunkIndices) const = 0;

        //! Calculate whether actor should be simulated by a static or dynamic rigid body. See more at
        //! BlastActorDesc::m_isStatic.
        virtual bool CalculateIsStatic(
            const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor,
            const AZStd::vector<uint32_t>& chunkIndices) const = 0;

        //! Calculate components that the entity simulating the actor should have on it. See more at
        //! BlastActorDesc::m_entity.
        virtual AZStd::vector<AZ::Uuid> CalculateComponents(bool isStatic) const = 0;
    };

    class BlastActorFactoryImpl : public BlastActorFactory
    {
    public:
        BlastActor* CreateActor(const BlastActorDesc& desc) override;
        void DestroyActor(BlastActor* actor) override;

        AZStd::vector<uint32_t> CalculateVisibleChunks(
            const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor) const override;
        bool CalculateIsLeafChunk(
            const Nv::Blast::TkActor& tkActor, const AZStd::vector<uint32_t>& chunkIndices) const override;
        bool CalculateIsStatic(
            const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor,
            const AZStd::vector<uint32_t>& chunkIndices) const override;
        AZStd::vector<AZ::Uuid> CalculateComponents(bool isStatic) const override;

    private:
        bool SupportGraphHasStaticActor(const BlastFamily& blastFamily, const Nv::Blast::TkActor& tkActor) const;
        bool VisibleChunksHasStaticActor(
            const BlastFamily& blastFamily, const AZStd::vector<uint32_t>& chunkIndices) const;
    };
} // namespace Blast
