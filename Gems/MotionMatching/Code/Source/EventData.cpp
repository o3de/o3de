/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <EventData.h>
#include <Allocators.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(DiscardFrameEventData, MotionEventAllocator)

    bool DiscardFrameEventData::Equal([[maybe_unused]]const EventData& rhs, [[maybe_unused]] bool ignoreEmptyFields) const
    {
        return true;
    }

    void DiscardFrameEventData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<DiscardFrameEventData, EventData>()
            ->Version(1)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<DiscardFrameEventData>("[Motion Matching] Discard Frame", "Event used for discarding ranges of the animation..")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ_CRC_CE("Creatable"), true)
            ;
    }

    ///////////////////////////////////////////////////////////////////////////

    AZ_CLASS_ALLOCATOR_IMPL(TagEventData, MotionEventAllocator)

    bool TagEventData::Equal(const EventData& rhs, [[maybe_unused]] bool ignoreEmptyFields) const
    {
        const TagEventData* other = azdynamic_cast<const TagEventData*>(&rhs);
        if (other)
        {
            return AZ::StringFunc::Equal(m_tag.c_str(), other->m_tag.c_str(), /*caseSensitive=*/false);
        }
        return false;
    }

    void TagEventData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<TagEventData, EventData>()
            ->Version(1)
            ->Field("tag", &TagEventData::m_tag)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<TagEventData>("[Motion Matching] Tag", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->Attribute(AZ_CRC_CE("Creatable"), true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &TagEventData::m_tag, "Tag", "The tag that should be active.")
            ;
    }
} // namespace EMotionFX::MotionMatching
