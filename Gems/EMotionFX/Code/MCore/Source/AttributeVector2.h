/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/Math/Vector2.h>
#include "StandardHeaders.h"
#include "Attribute.h"
#include "Vector.h"
#include "StringConversions.h"
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    /**
     * The Vector2 attribute class.
     * This attribute represents one Vector2.
     */
    class MCORE_API AttributeVector2
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeVector2, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000005
        };

        // The actual in-memory size of vector2 can depend on CPU architecture, use a hardcoded size instead of sizeof()
        static constexpr size_t sizeofVector2 = 8;

        static AttributeVector2* Create();
        static AttributeVector2* Create(const AZ::Vector2& value);
        static AttributeVector2* Create(float x, float y);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeofVector2; }

        // adjust values
        MCORE_INLINE const AZ::Vector2& GetValue() const                { return m_value; }
        MCORE_INLINE void SetValue(const AZ::Vector2& value)            { m_value = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeVector2::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeVector2"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributeVector2*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeVector2(valueString.c_str(), &m_value);
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, m_value); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeVector2); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_VECTOR2; }

    private:
        AZ::Vector2     m_value;     /**< The Vector2 value. */

        AttributeVector2()
            : Attribute(TYPE_ID)                    { m_value.Set(0.0f, 0.0f); }
        AttributeVector2(const AZ::Vector2& value)
            : Attribute(TYPE_ID)
            , m_value(value)     { }
        ~AttributeVector2() { }
    };
}   // namespace MCore
