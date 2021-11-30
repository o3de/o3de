/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomBackend.h>

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include "DomBackend.h"

namespace AZ::Dom
{
    Visitor::Result Backend::ReadFromStream(AZ::IO::GenericStream* stream, Visitor* visitor, size_t maxSize)
    {
        size_t length = stream->GetLength();
        if (length > maxSize)
        {
            return AZ::Failure(VisitorError(VisitorErrorCode::InternalError, "Stream is too large."));
        }
        AZStd::string buffer;
        buffer.resize(length);
        stream->Read(length, buffer.data());
        return ReadFromString(buffer, Lifetime::Temporary, visitor);
    }

    Visitor::Result Backend::ReadFromStringInPlace(AZStd::string& buffer, Visitor* visitor)
    {
        return ReadFromString(buffer, Lifetime::Persistent, visitor);
    }

    Visitor::Result Backend::WriteToStream(AZ::IO::GenericStream* stream, WriteCallback callback)
    {
        AZStd::unique_ptr<Visitor> writer = CreateStreamWriter(stream);
        return callback(writer.get());
    }

    Visitor::Result Backend::WriteToString(AZStd::string& buffer, WriteCallback callback)
    {
        AZ::IO::ByteContainerStream<AZStd::string> stream{&buffer};
        AZStd::unique_ptr<Visitor> writer = CreateStreamWriter(&stream);
        return callback(writer.get());
    }
}
