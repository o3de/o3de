/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BuilderComponent.h"
#include "ParticleBuilder.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <OpenParticleSystem/Asset/ParticleAssetHandler.h>
#include <Runtime/OpenParticleSystem/ParticleConfig.h>
#include <Runtime/OpenParticleSystem/ParticleEditDataConfig.h>
#include <Serializer/ParticleSourceData.h>

namespace OpenParticle
{
    void BuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
        ParticleConfig::Reflect(context);
        ParticleEditDataConfig::Reflect(context);
        ParticleSourceData::Reflect(context);
        ParticleAsset::Reflect(context);
    }

    BuilderComponent::BuilderComponent()
    {
    }

    BuilderComponent::~BuilderComponent()
    {
    }

    void BuilderComponent::Activate()
    {
        m_assetWorkers.emplace_back(MakeAssetBuilder<ParticleBuilder>());
        m_assetHandlers.emplace_back(MakeAssetHandler<ParticleAssetHandler>());
    }

    void BuilderComponent::Deactivate()
    {
        m_assetHandlers.clear();
        m_assetWorkers.clear();
    }
} // namespace OpenParticle
