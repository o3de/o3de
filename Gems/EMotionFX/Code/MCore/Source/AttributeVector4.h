/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/Math/Vector4.h>
#include "StandardHeaders.h"
#include "Attribute.h"
#include "StringConversions.h"
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    /**
     * The Vector4 attribute class.
     * This attribute represents one Vector4.
     */
    class MCORE_API AttributeVector4
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeVector4, AttributeAllocator, 0)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000007
        };

        static AttributeVector4* Create();
        static AttributeVector4* Create(const AZ::Vector4& value);
        static AttributeVector4* Create(float x, float y, float z, float w);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return sizeof(AZ::Vector4); }

        // adjust values
        MCORE_INLINE const AZ::Vector4& GetValue() const                { return mValue; }
        MCORE_INLINE void SetValue(const AZ::Vector4& value)            { mValue = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeVector4::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeVector4"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeVector4*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeVector4(valueString.c_str(), &mValue);
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, mValue); return true; }
        //      void ConvertCoordinateSystem()                              { GetCoordinateSystem().ConvertVector4(&mValue); }
        uint32 GetClassSize() const override                        { return sizeof(AttributeVector4); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_VECTOR4; }

    private:
        AZ::Vector4     mValue;     /**< The Vector4 value. */

        AttributeVector4()
            : Attribute(TYPE_ID)                    { mValue.Set(0.0f, 0.0f, 0.0f, 0.0f); }
        AttributeVector4(const AZ::Vector4& value)
            : Attribute(TYPE_ID)
            , mValue(value)     { }
        ~AttributeVector4() { }
    };
}   // namespace MCore
