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
#include <MCore/Source/LogManager.h>


namespace MCore
{
    /**
     * The pointer attribute class.
     * This attribute represents a pointer to an object. It cannot be loaded or saved to streams/files.
     * It is mainly used to temporarily store pointers to objects.
     */
    class MCORE_API AttributePointer
        : public Attribute
    {
        AZ_CLASS_ALLOCATOR(AttributePointer, AttributeAllocator)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x0000000c
        };

        static AttributePointer* Create(void* value = nullptr);

        // adjust values
        MCORE_INLINE void* GetValue() const                         { return m_value; }
        MCORE_INLINE void SetValue(void* value)                     { m_value = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributePointer::Create(m_value); }
        const char* GetTypeString() const override                  { return "AttributePointer"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            m_value = static_cast<const AttributePointer*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override     { MCORE_UNUSED(valueString); MCORE_ASSERT(false); return false; }   // currently unsupported
        bool ConvertToString(AZStd::string& outString) const override      { MCORE_UNUSED(outString); MCORE_ASSERT(false); return false; }     // currently unsupported
        size_t GetClassSize() const override                        { return sizeof(AttributePointer); }
        AZ::u32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        void*   m_value;     /**< The pointer value. */

        AttributePointer()
            : Attribute(TYPE_ID)
            , m_value(nullptr)   { }
        AttributePointer(void* pointer)
            : Attribute(TYPE_ID)
            , m_value(pointer)   { }
        ~AttributePointer() {}
    };
}   // namespace MCore
