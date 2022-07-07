/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <Components/BlastMeshDataComponent.h>

namespace Blast
{
    BlastMeshDataComponent::BlastMeshDataComponent(
        const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& meshAssets)
        : m_meshAssets(meshAssets)
    {
        for (auto& meshAsset : m_meshAssets)
        {
            meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
        }
    }

    void BlastMeshDataComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize
                ->Class<BlastMeshDataComponent, AZ::Component>()
                ->Version(2)
                ->Field("MeshAssets", &BlastMeshDataComponent::m_meshAssets);
        }
    }

    void BlastMeshDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("BlastMeshDataService"));
    }

    void BlastMeshDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("BlastMeshDataService"));
    }

    const AZ::Data::Asset<AZ::RPI::ModelAsset>& BlastMeshDataComponent::GetMeshAsset(size_t index) const
    {
        if (index < m_meshAssets.size())
        {
            return m_meshAssets[index];
        }

        AZ_Warning(
            "Blast", false, "Tried to get mesh asset at index %zu outside of Blast mesh data asset array.", index);
        static AZ::Data::Asset<AZ::RPI::ModelAsset> invalidAsset;
        return invalidAsset;
    }

    const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& BlastMeshDataComponent::GetMeshAssets() const
    {
        return m_meshAssets;
    }
} // namespace Blast
