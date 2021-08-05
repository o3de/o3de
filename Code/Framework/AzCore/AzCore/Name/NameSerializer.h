/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
