/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Serializer/ParticleSourceData.h>

namespace OpenParticleSystemEditor
{
    constexpr AZStd::string_view EMITTER_CONFIG = "Emitter Conifg";
    constexpr AZStd::string_view EMITTER_EMITS = "Emit Modules";
    constexpr AZStd::string_view EMITTER_SPAWNS = "Spawn Modules";
    constexpr AZStd::string_view EMITTER_UPDATES = "Update Modules";
    constexpr AZStd::string_view RENDER_TYPES = "Renderer";


    class ParticleDocumentRequest
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZStd::string;

        virtual OpenParticle::ParticleSourceData* GetParticleSourceData() = 0;
        virtual void NotifyParticleSourceDataModified() = 0;
        virtual OpenParticle::ParticleSourceData::DetailInfo* AddDetail(const AZStd::string&) = 0;
        virtual OpenParticle::ParticleSourceData::DetailInfo* CopyDetail(AZStd::string&) = 0;
        virtual void RemoveDetail(OpenParticle::ParticleSourceData::DetailInfo*) = 0;
        virtual OpenParticle::ParticleSourceData::DetailInfo* GetDetail() = 0;
        virtual OpenParticle::ParticleSourceData::EmitterInfo* GetEmitter() = 0;
        virtual OpenParticle::ParticleSourceData::DetailInfo* SetEmitterAndDetail(OpenParticle::ParticleSourceData::EmitterInfo* destEmitter, OpenParticle::ParticleSourceData::DetailInfo* destDetail) = 0;
        virtual void SetCopyName(AZStd::string copyName) = 0;
        virtual AZStd::string GetCopyWidgetName() = 0;
        virtual void ClearCopyCache() = 0;
    };

    using ParticleDocumentRequestBus = AZ::EBus<ParticleDocumentRequest>;

    class ParticleDocumentNotify
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        // used for emitter list
        virtual void OnParticleSourceDataLoaded([[maybe_unused]] OpenParticle::ParticleSourceData* particleAssetData, [[maybe_unused]] AZStd::string particleAssetPath) const {}
        // used for viewport
        virtual void OnDocumentOpened([[maybe_unused]] AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset, [[maybe_unused]] AZStd::string particleAssetPath) {}
        // used for clearing preview when emitter become invisible
        virtual void OnDocumentInvisible() {}
    };

    using ParticleDocumentNotifyBus = AZ::EBus<ParticleDocumentNotify>;
}
