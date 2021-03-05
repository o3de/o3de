/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "Endian.h"
#include "Stream.h"
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    namespace Network
    {
        class AnimGraphSnapshotChunkSerializer;
    }
}

namespace MCore
{
    // forward declarations
    class AttributeSettings;


    // the attribute interface types
    enum : uint32
    {
        ATTRIBUTE_INTERFACETYPE_FLOATSPINNER    = 0,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_FLOATSLIDER     = 1,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSPINNER      = 2,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSLIDER       = 3,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_COMBOBOX        = 4,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_CHECKBOX        = 5,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_VECTOR2         = 6,        // MCore::AttributeVector2
        ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO    = 7,        // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_VECTOR4         = 8,        // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_COLOR           = 10,       // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_STRING          = 11,       // MCore::AttributeString
        ATTRIBUTE_INTERFACETYPE_TAG             = 26,       // MCore::AttributeBool
        ATTRIBUTE_INTERFACETYPE_VECTOR3         = 113212,   // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_PROPERTYSET     = 113213,   // MCore::AttributeSet
        ATTRIBUTE_INTERFACETYPE_DEFAULT         = 0xFFFFFFFF// use the default attribute type that the specific attribute class defines as default
    };


    /**
     *
     *
     *
     */
    class MCORE_API Attribute
    {
        friend class AttributeFactory;
    public:
        virtual ~Attribute();

        virtual Attribute* Clone() const = 0;
        virtual const char* GetTypeString() const = 0;
        MCORE_INLINE uint32 GetType() const                                         { return mTypeID; }
        virtual bool InitFromString(const AZStd::string& valueString) = 0;
        virtual bool ConvertToString(AZStd::string& outString) const = 0;
        virtual bool InitFrom(const Attribute* other) = 0;
        virtual uint32 GetClassSize() const = 0;
        virtual uint32 GetDefaultInterfaceType() const = 0;

        // These two members and ReadData can go away once we put the old-format parser
        bool Read(Stream* stream, MCore::Endian::EEndianType sourceEndianType);
        virtual uint32 GetDataSize() const = 0;         // data only

        Attribute& operator=(const Attribute& other);

        virtual void NetworkSerialize(EMotionFX::Network::AnimGraphSnapshotChunkSerializer&) {};

    protected:
        uint32      mTypeID;    /**< The unique type ID of the attribute class. */

        Attribute(uint32 typeID);

        /**
         * Read the attribute info and data from a given stream.
         * Please note that the endian information of the actual data is not being converted. You have to handle that yourself.
         * The data endian conversion could be done with for example the static Attribute::ConvertDataEndian method.
         * @param stream The stream to read the info and data from.
         * @param endianType The endian type in which the data is stored in the stream.
         * @param version The version of the attribute.
         * @result Returns true when successful, or false when reading failed.
         */
        virtual bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) = 0;
    };
} // namespace MCore
