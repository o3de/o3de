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
     * The boolean attribute class.
     * This attribute represents one bool.
     */
    class MCORE_API AttributeBool
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000004
        };

        static AttributeBool* Create(bool value = false);

        // adjust values
        MCORE_INLINE bool GetValue() const                          { return mValue; }
        MCORE_INLINE void SetValue(bool value)                      { mValue = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return sizeof(bool); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeBool::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeBool"; }
        bool InitFrom(const Attribute* other);
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeBool(valueString.c_str(), &mValue);
        }
        bool ConvertToString(AZStd::string& outString) const override      { outString = AZStd::string::format("%d", (mValue) ? 1 : 0); return true; }
        uint32 GetClassSize() const override                        { return sizeof(AttributeBool); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_CHECKBOX; }

    private:
        bool    mValue;     /**< The boolean value, false on default. */

        AttributeBool()
            : Attribute(TYPE_ID)
            , mValue(false) {}
        AttributeBool(bool value)
            : Attribute(TYPE_ID)
            , mValue(value) {}
        ~AttributeBool() {}

        uint32 GetDataSize() const override                         { return sizeof(int8); }

        // read from a stream
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override
        {
            MCORE_UNUSED(version);
            MCORE_UNUSED(streamEndianType);
            int8 streamValue;
            if (stream->Read(&streamValue, sizeof(int8)) == 0)
            {
                return false;
            }

            mValue = (streamValue == 0) ? false : true;
            return true;
        }

    };
}   // namespace MCore
