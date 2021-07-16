/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/EventDataSyncable.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    EventDataSyncable::EventDataSyncable()
        : m_hash(AZStd::hash<AZ::TypeId>()(azrtti_typeid(this)))
    {
    }

    EventDataSyncable::EventDataSyncable(const size_t hash)
        : m_hash(hash)
    {
    }

    void EventDataSyncable::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EventDataSyncable, EventData>()
            ->Version(1)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<EventDataSyncable>("EventDataSyncable", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }

    size_t EventDataSyncable::HashForSyncing(bool /*isMirror*/) const
    {
        return m_hash;
    }
} // end namespace EMotionFX
