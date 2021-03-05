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

#include <EMotionFX/Source/ObjectId.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/string/conversions.h>


namespace EMotionFX
{
    const ObjectId ObjectId::InvalidId = ObjectId(0);

    ObjectId::ObjectId()
        : m_id(InvalidId)
    {}

    ObjectId::ObjectId(const ObjectId& id)
        : m_id(id)
    {}

    ObjectId::ObjectId(AZ::u64 id)
        : m_id(id)
    {}

    ObjectId ObjectId::Create()
    {
        AZ::u64 result = AZ::Sfmt::GetInstance().Rand64();

        while (ObjectId(result) == InvalidId)
        {
            result = AZ::Sfmt::GetInstance().Rand64();
        }

        return ObjectId(result);
    }

    ObjectId ObjectId::CreateFromString(const AZStd::string& text)
    {
        const AZ::u64 result = AZStd::stoull(text);

        if (result == 0 || result == ULLONG_MAX)
        {
            return ObjectId::InvalidId;
        }

        return result;
    }

    AZStd::string ObjectId::ToString() const
    {
        return AZStd::string::format("%llu", m_id);
    }

    bool ObjectId::IsValid() const
    {
        return m_id != InvalidId;
    }

    void ObjectId::SetInvalid()
    {
        m_id = InvalidId;
    }

    bool ObjectId::operator==(const ObjectId& rhs) const
    {
        return m_id == rhs.m_id;
    }

    bool ObjectId::operator!=(const ObjectId& rhs) const
    {
        return m_id != rhs.m_id;
    }
} // namespace EMotionFX