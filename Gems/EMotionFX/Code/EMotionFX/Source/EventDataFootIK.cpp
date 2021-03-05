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

#include <EMotionFX/Source/EventDataFootIK.h>
#include <EMotionFX/Source/Allocators.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventDataFootIK, MotionEventAllocator, 0)

    bool EventDataFootIK::Equal(const EventData& rhs, bool ignoreEmptyFields) const
    {
        AZ_UNUSED(ignoreEmptyFields);
        const EventDataFootIK* other = azdynamic_cast<const EventDataFootIK*>(&rhs);
        if (other)
        {
            return ((other->m_foot == m_foot) && 
                    (other->m_ikEnabled == m_ikEnabled) &&
                    (other->m_locked == m_locked));
        }
        return false;
    }

    void EventDataFootIK::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EventDataFootIK, EventData>()
            ->Version(1)
            ->Field("foot", &EventDataFootIK::m_foot)
            ->Field("ikEnabled", &EventDataFootIK::m_ikEnabled)
            ->Field("locked", &EventDataFootIK::m_locked)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<EventDataFootIK>("EventDataFootIK", "Footplant IK event data used to tell when IK should be be enabled and for which foot.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ_CRC("Creatable", 0x47bff8c4), true)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EventDataFootIK::m_foot, "Foot", "Which foot should have IK active?")
                ->EnumAttribute(Foot::Left, "Left Foot")
                ->EnumAttribute(Foot::Right, "Right Foot")
                ->EnumAttribute(Foot::Both, "Both Feet")
            ->DataElement(AZ::Edit::UIHandlers::Default, &EventDataFootIK::m_ikEnabled, "IK enabled", "Should the foot IK be enabled or disabled? Adding disabled events really only makes sense when the Foot IK node is set to automatic IK mode.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &EventDataFootIK::m_locked, "Foot locked", "Enable foot locking? This will freeze the position and rotation of the foot while this event is active.")
            ;
    }
}
