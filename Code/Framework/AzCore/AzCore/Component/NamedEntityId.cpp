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
#include <AzCore/Component/NamedEntityId.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    //=========================================================================
    // NamedEntityId
    //=========================================================================
    NamedEntityId::NamedEntityId()
        : m_entityName("<Unknown>")
    {
    }

    NamedEntityId::NamedEntityId(const AZ::EntityId& entityId, AZStd::string_view entityName)
        : EntityId(entityId)
        , m_entityName(entityName)
    {
        if (entityId.IsValid() && m_entityName.empty())
        {
            AZ::Entity* entity = nullptr;
            ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);

            if (entity)
            {
                m_entityName = entity->GetName();
            }
        }
    }

    void NamedEntityId::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NamedEntityId, EntityId>()
                ->Version(0)
                ->Field("name", &NamedEntityId::m_entityName)
                ;
        }
    }
    
    AZStd::string NamedEntityId::ToString() const
    {
        return AZStd::string::format("%s [%llu]", m_entityName.c_str(), static_cast<AZ::u64>(static_cast<AZ::EntityId>(*this)));
    }

    AZStd::string_view NamedEntityId::GetName() const
    {
        return m_entityName;
    }

    bool NamedEntityId::operator==(const NamedEntityId& rhs) const
    {
        return EntityId::operator==(static_cast<const EntityId&>(rhs));
    }

    bool NamedEntityId::operator==(const EntityId& rhs) const
    {
        return EntityId::operator==(rhs);
    }

    bool NamedEntityId::operator!=(const NamedEntityId& rhs) const
    {
        return EntityId::operator!=(static_cast<const EntityId&>(rhs));
    }

    bool NamedEntityId::operator!=(const EntityId& rhs) const
    {
        return EntityId::operator!=(rhs);
    }

    bool NamedEntityId::operator<(const NamedEntityId& rhs) const
    {
        return EntityId::operator<(static_cast<const EntityId&>(rhs));
    }

    bool NamedEntityId::operator<(const EntityId& rhs) const
    {
        return EntityId::operator<(rhs);
    }

    bool NamedEntityId::operator>(const NamedEntityId& rhs) const
    {
        return EntityId::operator>(static_cast<const EntityId&>(rhs));
    }

    bool NamedEntityId::operator>(const EntityId& rhs) const
    {
        return EntityId::operator>(rhs);
    }
}   // namespace AZ
