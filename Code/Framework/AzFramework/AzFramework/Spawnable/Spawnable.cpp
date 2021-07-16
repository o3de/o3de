/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    Spawnable::Spawnable(const AZ::Data::AssetId& id, AssetStatus status)
        : AZ::Data::AssetData(id, status)
    {
    }

    const Spawnable::EntityList& Spawnable::GetEntities() const
    {
        return m_entities;
    }

    Spawnable::EntityList& Spawnable::GetEntities()
    {
        return m_entities;
    }

    bool Spawnable::IsEmpty() const
    {
        return m_entities.empty();
    }

    SpawnableMetaData& Spawnable::GetMetaData()
    {
        return m_metaData;
    }

    const SpawnableMetaData& Spawnable::GetMetaData() const
    {
        return m_metaData;
    }

    void Spawnable::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<Spawnable, AZ::Data::AssetData>()->Version(1)
                ->Field("Meta data", &Spawnable::m_metaData)
                ->Field("Entities", &Spawnable::m_entities);
        }
    }
} // namespace AzFramework
