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
        MCORE_INLINE bool GetValue() const                          { return m_value; }
        MCORE_INLINE void SetValue(bool value)                      { m_value = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&m_value); }
        MCORE_INLINE size_t GetRawDataSize() const                  { return sizeof(bool); }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeBool::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributeBool"; }
        bool InitFrom(const Attribute* other) override;
        bool InitFromString(const AZStd::string& valueString) override
        {
            return AzFramework::StringFunc::LooksLikeBool(valueString.c_str(), &m_value);
        }
        bool ConvertToString(AZStd::string& outString) const override      { outString = AZStd::string::format("%d", (m_value) ? 1 : 0); return true; }
        size_t GetClassSize() const override                        { return sizeof(AttributeBool); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_CHECKBOX; }

    private:
        bool    m_value;     /**< The boolean value, false on default. */

        AttributeBool()
            : Attribute(TYPE_ID)
            , m_value(false) {}
        AttributeBool(bool value)
            : Attribute(TYPE_ID)
            , m_value(value) {}
        ~AttributeBool() {}
    };
}   // namespace MCore
