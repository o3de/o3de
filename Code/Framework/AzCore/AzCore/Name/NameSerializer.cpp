/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/NameSerializer.h>
#include <AzCore/IO/GenericStreams.h>

namespace AZ
{
    size_t NameSerializer::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool)
    {
        const size_t dataSize = static_cast<size_t>(in.GetLength());

        AZStd::string outText;
        outText.resize_no_construct(dataSize);
        in.Read(dataSize, outText.data());

        return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
    }

    size_t NameSerializer::TextToData(const char* text, unsigned int, IO::GenericStream& stream, bool)
    {
        const size_t bytesToWrite = strlen(text);
        return static_cast<size_t>(stream.Write(bytesToWrite, reinterpret_cast<const void*>(text)));
    }

    size_t NameSerializer::Save(const void* classPtr, IO::GenericStream& stream, bool /*isDataBigEndian*/)
    {
        const Name* azName = reinterpret_cast<const Name*>(classPtr);
        return static_cast<size_t>(stream.Write(azName->GetStringView().size(), azName->GetStringView().data()));
    }

    bool NameSerializer::Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool /*isDataBigEndian*/)
    {
        AZStd::string nameStr;
        size_t textLen = static_cast<size_t>(stream.GetLength());
        nameStr.resize_no_construct(textLen);
        stream.Read(textLen, nameStr.data());

        Name* azName = reinterpret_cast<Name*>(classPtr);
        *azName = nameStr;

        return true;
    }

    bool NameSerializer::CompareValueData(const void* lhs, const void* rhs)
    {
        const Name& lName = *(reinterpret_cast<const Name*>(lhs));
        const Name& rName = *(reinterpret_cast<const Name*>(rhs));
        return lName == rName;
    }
} // namespace AZ

