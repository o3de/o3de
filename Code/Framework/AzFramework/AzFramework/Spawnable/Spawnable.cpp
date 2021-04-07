/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    Spawnable::Spawnable(Spawnable&& other)
        : m_entities(AZStd::move(other.m_entities))
    {
    }


    Spawnable& Spawnable::operator=(Spawnable&& other)
    {
        if (this != &other)
        {
            m_entities = AZStd::move(other.m_entities);
        }

        return *this;
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
