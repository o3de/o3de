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
        AZ_CLASS_ALLOCATOR(AttributeVector3, AttributeAllocator, 0)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000006
        };

        static AttributeVector3* Create();
        static AttributeVector3* Create(const AZ::Vector3& value);
        static AttributeVector3* Create(float x, float y, float z);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return sizeof(AZ::Vector3); }

        // adjust values
        MCORE_INLINE const AZ::Vector3& GetValue() const     { return mValue; }
        MCORE_INLINE void SetValue(const AZ::Vector3& value) { mValue = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeVector3::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeVector3"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeVector3*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override
        {
            AZ::Vector3 vec3;
            if (!AzFramework::StringFunc::LooksLikeVector3(valueString.c_str(), &vec3))
            {
                return false;
            }
            mValue.Set(vec3.GetX(), vec3.GetY(), vec3.GetZ());
            return true;
        }
        bool ConvertToString(AZStd::string& outString) const override      { AZStd::to_string(outString, mValue); return true; }
        uint32 GetClassSize() const override                        { return sizeof(AttributeVector3); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_VECTOR3; }

    private:
        AZ::Vector3  mValue;     /**< The Vector3 value. */

        AttributeVector3()
            : Attribute(TYPE_ID)                    { mValue.Set(0.0f, 0.0f, 0.0f); }
        AttributeVector3(const AZ::Vector3& value)
            : Attribute(TYPE_ID)
            , mValue(value)     { }
        ~AttributeVector3() { }

        uint32 GetDataSize() const override                         { return sizeof(AZ::Vector3); }

        // read from a stream
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override
        {
            MCORE_UNUSED(version);

            // read the value
            AZ::PackedVector3f streamValue(0.0f);
            if (stream->Read(&streamValue, sizeof(AZ::PackedVector3f)) == 0)
            {
                return false;
            }

            // convert endian
            mValue = AZ::Vector3(streamValue.GetX(), streamValue.GetY(), streamValue.GetZ());
            Endian::ConvertVector3(&mValue, streamEndianType);

            return true;
        }

    };
}   // namespace MCore
