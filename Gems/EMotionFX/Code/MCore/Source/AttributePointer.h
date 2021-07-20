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
        AZ_CLASS_ALLOCATOR(AttributePointer, AttributeAllocator, 0)

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x0000000c
        };

        static AttributePointer* Create(void* value = nullptr);

        // adjust values
        MCORE_INLINE void* GetValue() const                         { return mValue; }
        MCORE_INLINE void SetValue(void* value)                     { mValue = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributePointer::Create(mValue); }
        const char* GetTypeString() const override                  { return "AttributePointer"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributePointer*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const AZStd::string& valueString) override     { MCORE_UNUSED(valueString); MCORE_ASSERT(false); return false; }   // currently unsupported
        bool ConvertToString(AZStd::string& outString) const override      { MCORE_UNUSED(outString); MCORE_ASSERT(false); return false; }     // currently unsupported
        uint32 GetClassSize() const override                        { return sizeof(AttributePointer); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        void*   mValue;     /**< The pointer value. */

        AttributePointer()
            : Attribute(TYPE_ID)
            , mValue(nullptr)   { }
        AttributePointer(void* pointer)
            : Attribute(TYPE_ID)
            , mValue(pointer)   { }
        ~AttributePointer() {}

        uint32 GetDataSize() const override                         { return sizeof(void*); }

        // read from a stream
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override
        {
            MCORE_UNUSED(stream);
            MCORE_UNUSED(streamEndianType);
            MCORE_UNUSED(version);

            MCore::LogWarning("MCore::AttributePointer::ReadData() - Pointer attributes cannot be read from disk.");
            return false;
        }
    };
}   // namespace MCore
