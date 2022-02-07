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
     * The signed 32 bit integer attribute class.
     * This attribute represents one signed int.
     */
    class MCORE_API AttributeInt32
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000002
        };

        static AttributeInt32* Create(int32 value = 0);

        // adjust values
        MCORE_INLINE int32 GetValue() const                         { return m_value; }
        MCORE_INLINE void SetValue(int32 value)                     { m_value = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(int32); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeInt32::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeInt32"; }
        bool InitFrom(const Attribute* other) override;
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeInt(valueString.c_str(), &m_value);
        }
        bool ConvertToString(AZStd::string& outString) const override      { outString = AZStd::string::format("%d", m_value); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeInt32); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_INTSPINNER; }

    private:
        int32   m_value;     /**< The signed integer value. */

        AttributeInt32()
            : Attribute(TYPE_ID)
            , m_value(0)  {}
        AttributeInt32(int32 value)
            : Attribute(TYPE_ID)
            , m_value(value) {}
        ~AttributeInt32() {}
    };
}   // namespace MCore
