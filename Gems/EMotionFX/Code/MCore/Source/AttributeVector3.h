/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "Attribute.h"
#include "StringConversions.h"
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    /**
     * The Vector3 attribute class.
     * This attribute represents one Vector3.
     */
    class MCORE_API AttributeVector3
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeVector3, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000006
        };

        static AttributeVector3* Create();
        static AttributeVector3* Create(const AZ::Vector3& value);
        static AttributeVector3* Create(float x, float y, float z);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(AZ::Vector3); }

        // adjust values
        MCORE_INLINE const AZ::Vector3& GetValue() const     { return m_value; }
        MCORE_INLINE void SetValue(const AZ::Vector3& value) { m_value = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeVector3::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeVector3"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributeVector3*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            AZ::Vector3 vec3;
            if (!AzFramework::StringFunc::LooksLikeVector3(valueString.c_str(), &vec3))
            {
                return false;
            }
            m_value.Set(vec3.GetX(), vec3.GetY(), vec3.GetZ());
            return true;
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, m_value); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeVector3); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_VECTOR3; }

    private:
        AZ::Vector3  m_value;     /**< The Vector3 value. */

        AttributeVector3()
            : Attribute(TYPE_ID)                    { m_value.Set(0.0f, 0.0f, 0.0f); }
        AttributeVector3(const AZ::Vector3& value)
            : Attribute(TYPE_ID)
            , m_value(value)     { }
        ~AttributeVector3() { }
    };
}   // namespace MCore
