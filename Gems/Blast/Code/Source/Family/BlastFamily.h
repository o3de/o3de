/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Actor/BlastActorFactory.h>
#include <Actor/EntityProvider.h>
#include <Asset/BlastAsset.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Blast/BlastActor.h>
#include <Blast/BlastActorConfiguration.h>
#include <Blast/BlastDebug.h>
#include <Material/BlastMaterial.h>
#include <Common/BlastInterfaces.h>

struct NvBlastExtMaterial;

namespace Nv
{
    namespace Blast
    {
        struct TkEvent;
        class ExtPxAsset;
        class TkFamily;
        class TkGroup;
    } // namespace Blast
} // namespace Nv

namespace Blast
{
    class ActorTracker;

    //! Set of options used to create a Blast Family
    struct BlastFamilyDesc
    {
        BlastAsset& m_asset; //!< Blast asset to create from.
        BlastListener* m_listener = nullptr; //!< Blast listener to notify about actor creations/destructions, this would generally be BlastFamilyComponent instance.
        Nv::Blast::TkGroup* m_group = nullptr; //!< if not nullptr created TkActor (and TkFamily) will be placed in this group
        Physics::MaterialId m_physicsMaterial;
        const Material* m_blastMaterial = nullptr;
        AZStd::shared_ptr<BlastActorFactory> m_actorFactory;
        AZStd::shared_ptr<EntityProvider> m_entityProvider;
        const BlastActorConfiguration& m_actorConfiguration;
    };

    //! Listens to events from the Blast toolkit family and manages BlastActors for a destructible object.
    class BlastFamily
    {
    public:
        //! Constructor for the BlastFamily
        //! @param desc Creation descriptor
        static AZStd::unique_ptr<BlastFamily> Create(const BlastFamilyDesc& desc);
        virtual ~BlastFamily() = default;

        //! Class allocator to use aznew
        AZ_CLASS_ALLOCATOR(BlastFamily, AZ::SystemAllocator, 0);

        //! Spawns this BlastFamily, creating the initial actors.
        //! @param transform Initial transform of the destructible object.
        virtual bool Spawn(const AZ::Transform& transform) = 0;

        //! Despawns this BlastFamily, destroying all created actors.
        virtual void Despawn() = 0;

        virtual void HandleEvents(const Nv::Blast::TkEvent* events, uint32_t eventCount) = 0;
        virtual void DestroyActor(BlastActor* blastActor) = 0;

        virtual ActorTracker& GetActorTracker() = 0;
        virtual const Nv::Blast::TkFamily* GetTkFamily() const = 0;
        virtual Nv::Blast::TkFamily* GetTkFamily() = 0;
        virtual const Nv::Blast::ExtPxAsset& GetPxAsset() const = 0;
        virtual const BlastActorConfiguration& GetActorConfiguration() const = 0;

        virtual void FillDebugRender(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode, float renderScale) = 0;
    };
} // namespace Blast
