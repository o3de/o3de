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
#include <AzCore/Memory/Memory.h>


namespace MCore
{
    /**
     * The float attribute class.
     * This attribute represents one float.
     */
    class MCORE_API AttributeFloat
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        static AttributeFloat* Create(float value = 0.0f);

        // adjust values
        MCORE_INLINE float GetValue() const                         { return mValue; }
        MCORE_INLINE void SetValue(float value)                     { mValue = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(float); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeFloat::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeFloat"; }
        bool InitFrom(const Attribute* other) override;
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeFloat(valueString.c_str(), &mValue);
        }
        bool ConvertToString(AZStd::string& outString) const override      { outString = AZStd::string::format("%.8f", mValue); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeFloat); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_FLOATSPINNER; }

    private:
        float   mValue;     /**< The float value. */

        AttributeFloat()
            : Attribute(TYPE_ID)
            , mValue(0.0f)  {}
        AttributeFloat(float value)
            : Attribute(TYPE_ID)
            , mValue(value) {}
        ~AttributeFloat() {}
    };
}   // namespace MCore
