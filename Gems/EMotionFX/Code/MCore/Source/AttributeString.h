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
#include <MCore/Source/AttributeAllocator.h>


namespace MCore
{
    /**
     * The string attribute class.
     * This attribute represents one string.
     */
    class MCORE_API AttributeString
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributeString, AttributeAllocator, 0)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        static AttributeString* Create(const AZStd::string& value);
        static AttributeString* Create(const char* value = "");

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(mValue.data()); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return static_cast<uint32>(mValue.size()); }

        // adjust values
        MCORE_INLINE const char* AsChar() const                     { return mValue.c_str(); }
        MCORE_INLINE const AZStd::string& GetValue() const          { return mValue; }
        MCORE_INLINE void SetValue(const AZStd::string& value)      { mValue = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeString::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributeString"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeString*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override     { mValue = valueString; return true; }
        bool ConvertToString(AZStd::string& outString) const override      { outString = mValue; return true; }
        uint32 GetClassSize() const override                        { return sizeof(AttributeString); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_STRING; }

    private:
        AZStd::string   mValue;     /**< The string value. */

        AttributeString()
            : Attribute(TYPE_ID)  { }
        AttributeString(const AZStd::string& value)
            : Attribute(TYPE_ID)
            , mValue(value) { }
        AttributeString(const char* value)
            : Attribute(TYPE_ID)
            , mValue(value) { }
        ~AttributeString()                              { mValue.clear(); }
    };
}   // namespace MCore
