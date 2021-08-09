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
#include "Color.h"
#include "StringConversions.h"
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
        AZ_CLASS_ALLOCATOR(AttributeColor, AttributeAllocator, 0)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x0000000a
        };

        static AttributeColor* Create();
        static AttributeColor* Create(const RGBAColor& value);

        // adjust values
        MCORE_INLINE const RGBAColor& GetValue() const              { return mValue; }
        MCORE_INLINE void SetValue(const RGBAColor& value)          { mValue = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(RGBAColor); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeColor::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeColor"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeColor*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            AZ::Vector4 vec4;
            if (!AzFramework::StringFunc::LooksLikeVector4(valueString.c_str(), &vec4))
            {
                return false;
            }
            mValue.Set(vec4.GetX(), vec4.GetY(), vec4.GetZ(), vec4.GetW());
            return true;
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, AZ::Vector4(mValue.r, mValue.g, mValue.b, mValue.a)); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeColor); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_COLOR; }

    private:
        RGBAColor   mValue;     /**< The color value. */

        AttributeColor()
            : Attribute(TYPE_ID)                    { mValue.Set(0.0f, 0.0f, 0.0f, 1.0f); }
        AttributeColor(const RGBAColor& value)
            : Attribute(TYPE_ID)
            , mValue(value)     { }
        ~AttributeColor() {}
    };
}   // namespace MCore
