/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <OpenParticleSystem/Asset/ParticleAsset.h>

namespace OpenParticle
{
    class EditorParticleRequest
        : public AZ::ComponentBus
    {
    public:
        virtual void SetParticleAsset(AZ::Data::Asset<ParticleAsset> particleAsset, bool inParticleEditor) = 0;
        virtual void SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath) = 0;
    };

    using EditorParticleRequestBus = AZ::EBus<EditorParticleRequest>;
} // namespace OpenParticle
