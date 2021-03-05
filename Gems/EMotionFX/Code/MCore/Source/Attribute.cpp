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
