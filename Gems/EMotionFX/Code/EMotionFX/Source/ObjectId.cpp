/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Sfmt.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <EMotionFX/Source/ObjectId.h>

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

        if (result == 0 || result == AZStd::numeric_limits<unsigned long long>::max())
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

    void ObjectId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ObjectId>()->Version(1)->Field("id", &ObjectId::m_id);
    }
} // namespace EMotionFX
