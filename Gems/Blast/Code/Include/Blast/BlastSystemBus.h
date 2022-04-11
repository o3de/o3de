/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Blast/BlastDebug.h>
#include <Blast/BlastMaterial.h>

struct NvBlastExtRadialDamageDesc;
struct NvBlastExtProgramParams;

namespace Nv::Blast
{
    class ExtSerialization;
    class TkFramework;
    class TkGroup;
} // namespace Nv::Blast

namespace Blast
{
    struct BlastGlobalConfiguration
    {
        AZ_TYPE_INFO(BlastGlobalConfiguration, "{0B9DB6DD-0008-4EF6-9D75-141061144353}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<Blast::BlastMaterialLibraryAsset> m_materialLibrary = AZ::Data::AssetLoadBehavior::NoLoad;
        uint32_t m_stressSolverIterations = 180;
    };

    class BlastSystemRequests : public AZ::EBusTraits
    {
    public:
        AZ_TYPE_INFO(BlastSystemRequests, "{1CD63978-B0DA-40D4-8E1E-12AECC21039A}");

        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        BlastSystemRequests() = default;
        virtual ~BlastSystemRequests() = default;

        //! AZ::Interface requires these to be deleted.
        BlastSystemRequests(BlastSystemRequests&&) = delete;
        BlastSystemRequests& operator=(BlastSystemRequests&&) = delete;

        using MutexType = AZStd::mutex;

        //! Getters for the NvBlast singletons
        virtual Nv::Blast::TkFramework* GetTkFramework() const = 0;
        virtual Nv::Blast::ExtSerialization* GetExtSerialization() const = 0;
        virtual Nv::Blast::TkGroup* CreateTkGroup() = 0;

        //! Configuration
        virtual const BlastGlobalConfiguration& GetGlobalConfiguration() const = 0;
        virtual void SetGlobalConfiguration(const BlastGlobalConfiguration& globalConfiguration) = 0;

        //! Adds a damage description to the queue to be processed next tick.
        virtual void AddDamageDesc(AZStd::unique_ptr<NvBlastExtRadialDamageDesc> desc) = 0;
        virtual void AddDamageDesc(AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc> desc) = 0;
        virtual void AddDamageDesc(AZStd::unique_ptr<NvBlastExtShearDamageDesc> desc) = 0;
        virtual void AddDamageDesc(AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc> desc) = 0;
        virtual void AddDamageDesc(AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc> desc) = 0;

        //! Adds a damage shader's program parameters to the queue to be processed next tick.
        virtual void AddProgramParams(AZStd::unique_ptr<NvBlastExtProgramParams> program) = 0;

        virtual void SetDebugRenderMode(DebugRenderMode debugRenderMode) = 0;
    };
    using BlastSystemRequestBus = AZ::EBus<BlastSystemRequests>;
} // namespace Blast
