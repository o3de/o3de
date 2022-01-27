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

#include <MotionMatchEventData.h>
#include <Allocators.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/StringFunc/StringFunc.h>


namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(MotionMatchEventData, MotionEventAllocator, 0)

        bool MotionMatchEventData::Equal(const EventData& rhs, bool ignoreEmptyFields) const
        {
            AZ_UNUSED(ignoreEmptyFields);
            const MotionMatchEventData* other = azdynamic_cast<const MotionMatchEventData*>(&rhs);
            if (other)
            {
                return AZ::StringFunc::Equal(m_tag.c_str(), other->m_tag.c_str(), /*caseSensitive=*/false);
            }
            return false;
        }

        bool MotionMatchEventData::GetDiscardFrames() const
        {
            return AZ::StringFunc::Equal(m_tag.c_str(), "discard", /*caseSensitive=*/false);
        }

        void MotionMatchEventData::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<MotionMatchEventData, EventData>()
                ->Version(1)
                ->Field("tag", &MotionMatchEventData::m_tag)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<MotionMatchEventData>("MotionMatchEventData", "Motion matching related event data.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ_CRC("Creatable", 0x47bff8c4), true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &MotionMatchEventData::m_tag, "Tag", "The tag that should be active.")
                ;
        }
    } // namespace MotionMatching
} // namespace EMotionFX
