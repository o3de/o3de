/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Attribute.h"
#include "AttributeFactory.h"
#include "AttributeString.h"
#include "StringConversions.h"

namespace MCore
{
    // constructor
    Attribute::Attribute(uint32 typeID)
    {
        mTypeID = typeID;
    }


    // destructor
    Attribute::~Attribute()
    {
    }


    // equal operator
    Attribute& Attribute::operator=(const Attribute& other)
    {
        if (&other != this)
        {
            InitFrom(&other);
        }
        return *this;
    }

    // read the attribute
    bool Attribute::Read(Stream* stream, Endian::EEndianType sourceEndianType)
    {
        // read the version
        uint8 version;
        if (stream->Read(&version, sizeof(uint8)) == 0)
        {
            return false;
        }

        // read the data
        const bool result = ReadData(stream, sourceEndianType, version);
        if (result == false)
        {
            return false;
        }

        return true;
    }
    
}   // namespace MCore
