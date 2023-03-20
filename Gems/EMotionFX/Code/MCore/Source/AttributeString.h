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
        AZ_CLASS_ALLOCATOR(AttributeString, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        static AttributeString* Create(const AZStd::string& value);
        static AttributeString* Create(const char* value = "");

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(m_value.data()); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return m_value.size(); }

        // adjust values
        MCORE_INLINE const char* AsChar() const                     { return m_value.c_str(); }
        MCORE_INLINE const AZStd::string& GetValue() const          { return m_value; }
        MCORE_INLINE void SetValue(const AZStd::string& value)      { m_value = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeString::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeString"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributeString*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override     { m_value = valueString; return true; }
        bool ConvertToString(AZStd::string& outString) const override      { outString = m_value; return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeString); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_STRING; }

    private:
        AZStd::string   m_value;     /**< The string value. */

        AttributeString()
            : Attribute(TYPE_ID)  { }
        AttributeString(const AZStd::string& value)
            : Attribute(TYPE_ID)
            , m_value(value) { }
        AttributeString(const char* value)
            : Attribute(TYPE_ID)
            , m_value(value) { }
        ~AttributeString()                              { m_value.clear(); }
    };
}   // namespace MCore
