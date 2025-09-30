/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <OpenParticleSystem/ParticleComponentConfig.h>
#include <OpenParticleSystem/ParticleConfig.h>

namespace OpenParticle
{
    void ParticleComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleComponentConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("particleAsset", &ParticleComponentConfig::m_particleAsset)
                ->Field("enable", &ParticleComponentConfig::m_enable)
                ->Field("followActiveCamera", &ParticleComponentConfig::m_followActiveCamera)
                ->Field("autoPlay", &ParticleComponentConfig::m_autoPlay);
        }
    }
} // namespace OpenParticle
