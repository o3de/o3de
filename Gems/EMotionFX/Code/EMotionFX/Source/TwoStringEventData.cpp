/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Allocators.h"
#include "TwoStringEventData.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(TwoStringEventData, MotionEventAllocator)

    bool StringEqual(const AZStd::string& lhs, const AZStd::string& rhs, bool ignoreEmpty)
    {
        if (ignoreEmpty && lhs.empty())
        {
            return true;
        }
        return lhs == rhs;
    }

    void TwoStringEventData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<TwoStringEventData, EventDataSyncable>()
            ->Version(1)
            ->Field("subject", &TwoStringEventData::m_subject)
            ->Field("parameters", &TwoStringEventData::m_parameters)
            ->Field("mirrorSubject", &TwoStringEventData::m_mirrorSubject)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<TwoStringEventData>("TwoStringEventData", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ_CRC_CE("Creatable"), true)
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &TwoStringEventData::m_subject, "Subject", "")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &TwoStringEventData::m_parameters, "Parameters", "")
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &TwoStringEventData::m_mirrorSubject, "Mirror Subject", "")
            ;
    }

    const AZStd::string& TwoStringEventData::GetSubject() const
    {
        return m_subject;
    }

    const AZStd::string& TwoStringEventData::GetParameters() const
    {
        return m_parameters;
    }

    const AZStd::string& TwoStringEventData::GetMirrorSubject() const
    {
        return m_mirrorSubject;
    }

    bool TwoStringEventData::Equal(const EventData& rhs, bool ignoreEmptyFields) const
    {
        const TwoStringEventData* rhsStringData = azdynamic_cast<const TwoStringEventData*>(&rhs);
        if (rhsStringData)
        {
            return StringEqual(m_subject, rhsStringData->m_subject, ignoreEmptyFields)
                && StringEqual(m_parameters, rhsStringData->m_parameters, ignoreEmptyFields)
                && StringEqual(m_mirrorSubject, rhsStringData->m_mirrorSubject, ignoreEmptyFields);
        }
        return false;
    }

    size_t TwoStringEventData::HashForSyncing(bool isMirror) const
    {
        if (m_hash == 0)
        {
            m_hash = AZStd::hash<AZStd::string>()(m_subject);
            m_mirrorHash = AZStd::hash<AZStd::string>()(m_mirrorSubject);
        }
        return isMirror ? m_mirrorHash : m_hash;
    }
} // end namespace EMotionFX
