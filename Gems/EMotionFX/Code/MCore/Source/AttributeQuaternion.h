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
#include "Vector.h"
#include "StringConversions.h"
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    /**
     * The Vector4 attribute class.
     * This attribute represents one Vector4.
     */
    class MCORE_API AttributeQuaternion
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeQuaternion, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000008
        };

        static AttributeQuaternion* Create();
        static AttributeQuaternion* Create(float x, float y, float z, float w);
        static AttributeQuaternion* Create(const AZ::Quaternion& value);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(AZ::Quaternion); }

        // adjust values
        MCORE_INLINE const AZ::Quaternion& GetValue() const             { return m_value; }
        MCORE_INLINE void SetValue(const AZ::Quaternion& value)         { m_value = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeQuaternion::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeQuaternion"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributeQuaternion*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            AZ::Vector4 vec4;
            if (!AzFramework::StringFunc::LooksLikeVector4(valueString.c_str(), &vec4))
            {
                return false;
            }
            m_value.Set(vec4.GetX(), vec4.GetY(), vec4.GetZ(), vec4.GetW());
            return true;
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, m_value); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeQuaternion); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        AZ::Quaternion  m_value;     /**< The Quaternion value. */

        AttributeQuaternion()
            : Attribute(TYPE_ID)
            , m_value(AZ::Quaternion::CreateIdentity())
        {}
        AttributeQuaternion(const AZ::Quaternion& value)
            : Attribute(TYPE_ID)
            , m_value(value) {}
        ~AttributeQuaternion() { }
    };
}   // namespace MCore
