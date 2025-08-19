/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <OpenParticleSystem/Asset/ParticleAsset.h>

namespace OpenParticle
{
    class ParticleRequest
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZ::EntityId;

        virtual void Play() = 0;
        virtual void Pause() = 0;
        virtual void Stop() = 0;
        virtual void SetVisibility(bool visible) = 0;
        virtual bool GetVisibility() const = 0;

        virtual void SetParticleAsset(AZ::Data::Asset<ParticleAsset> particleAsset, bool inParticleEditor) = 0;
        virtual void SetParticleAssetByPath(AZStd::string path) = 0;
        virtual AZStd::string GetParticleAssetPath() const = 0;

        virtual void SetMaterialDiffuseMap(uint32_t emitterIndex, AZStd::string mapPath) = 0;
    };

    using ParticleRequestBus = AZ::EBus<ParticleRequest>;
} // namespace OpenParticle
