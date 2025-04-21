/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EventDataFloatArray.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventDataFloatArray, MotionEventAllocator)

    bool EventDataFloatArray::Equal(const EventData& rhs, bool ignoreEmptyFields) const
    {
        AZ_UNUSED(ignoreEmptyFields);
        const EventDataFloatArray* other = azdynamic_cast<const EventDataFloatArray*>(&rhs);
        if (other)
        {
            return other->m_floats == m_floats;
        }
        return false;
    }

    void EventDataFloatArray::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EventDataFloatArray, EventData>()
            ->Version(1)
            ->Field("subject", &EventDataFloatArray::m_subject)
            ->Field("floats", &EventDataFloatArray::m_floats);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext
            ->Class<EventDataFloatArray>(
                "EventDataFloatArray", "The event data holds a flex size float array.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->Attribute(AZ_CRC_CE("Creatable"), true)
            ->DataElement(
                AZ::Edit::UIHandlers::Default,
                &EventDataFloatArray::m_subject,
                "Subject",
                "The subject of this event.")
            ->DataElement(
                AZ::Edit::UIHandlers::Default,
                &EventDataFloatArray::m_floats,
                "Float Array",
                "The array of floats this event contains.");
    }

    float EventDataFloatArray::GetElement(size_t index) const
    {
        return m_floats[index];
    }

    AZStd::string EventDataFloatArray::DataToString() const
    {
        AZStd::string result;
        for (size_t i = 0; i < m_floats.size(); ++i)
        {
            if (i != 0)
            {
                result += ",";
            }
            result += AZStd::string::format("%.2f", m_floats[i]);
        }
        return result;
    }
} // namespace EMotionFX
