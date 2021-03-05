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

#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    //! Provides flattened serialization of Name objects as a simple string.
    class NameSerializer : public SerializeContext::IDataSerializer
    {
    public:

        //! Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool) override;

        //! Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t TextToData(const char* text, unsigned int, IO::GenericStream& stream, bool) override;

        //! Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool /*isDataBigEndian*/) override;

        //! Load the class data from a stream.
        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool /*isDataBigEndian*/) override;

        //! Compares two instances of the type.
        //! Input pointers are assumed to point to valid instances of the class.
        //! @return true if they match.
        bool CompareValueData(const void* lhs, const void* rhs) override;
    };

} // namespace AZ
