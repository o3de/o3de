/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "Attribute.h"
#include "StandardHeaders.h"

#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <MCore/Source/AttributeAllocator.h>

namespace MCore
{
    /**
     * The Color attribute class.
     * This attribute represents one Color.
     */
    class MCORE_API AttributeColor
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeColor, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x0000000a
        };

        static AttributeColor* Create();
        static AttributeColor* Create(const AZ::Color& value);

        // adjust values
        MCORE_INLINE const AZ::Color& GetValue() const              { return m_value; }
        MCORE_INLINE void SetValue(const AZ::Color& value)          { m_value = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(AZ::Color); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeColor::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeColor"; }

        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributeColor*>(other)->GetValue();
            return true;
        }

        bool InitFromString(const AZStd::string& valueString) override
        {
            AZ::Vector4 vec4;
            if (!AZ::StringFunc::LooksLikeVector4(valueString.c_str(), &vec4))
            {
                return false;
            }
            m_value.Set(vec4.GetX(), vec4.GetY(), vec4.GetZ(), vec4.GetW());
            return true;
        }
        
        bool ConvertToString(AZStd::string& outString) const override
        {
            AZStd::to_string(outString, m_value);
            return true;
        }
        
        size_t GetClassSize() const override
        {
            return sizeof(AttributeColor);
        }

        AZ::u32 GetDefaultInterfaceType() const override
        {
            return ATTRIBUTE_INTERFACETYPE_COLOR;
        }

    private:
        AZ::Color   m_value;     /**< The color value. */

        AttributeColor()
            : Attribute(TYPE_ID)                    { m_value.Set(0.0f, 0.0f, 0.0f, 1.0f); }
        AttributeColor(const AZ::Color& value)
            : Attribute(TYPE_ID)
            , m_value(value)     { }
        ~AttributeColor() {}
    };
}   // namespace MCore
