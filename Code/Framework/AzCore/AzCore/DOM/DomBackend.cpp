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

namespace AZ::DOM
{
    Visitor::Result Backend::ReadFromPath(AZ::IO::PathView pathName, Visitor* visitor, size_t maxFileSize)
    {
        auto readResult = AZ::Utils::ReadFile(pathName.Native(), maxFileSize);
        if (!readResult.IsSuccess())
        {
            return AZ::Failure(VisitorError(VisitorErrorCode::InternalError, readResult.TakeError()));
        }

        AZStd::string fileContents = readResult.TakeValue();
        return ReadFromString(fileContents, Lifetime::Temporary, visitor);
    }

    Visitor::Result Backend::ReadFromStream(AZ::IO::GenericStream* stream, Visitor* visitor, size_t maxSize)
    {
        size_t length = stream->GetLength();
        if (length > maxSize)
        {
            return AZ::Failure(VisitorError(VisitorErrorCode::InternalError, "Stream is too large."));
        }
        AZStd::string buffer;
        buffer.resize(maxSize);
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

    Visitor::Result Backend::WriteToPath(AZ::IO::PathView pathName, WriteCallback callback)
    {
        AZStd::string buffer;
        auto serializeResult = WriteToString(buffer, callback);
        if (!serializeResult.IsSuccess())
        {
            return serializeResult;
        }

        auto writeResult = AZ::Utils::WriteFile(buffer, pathName.Native());
        if (!writeResult.IsSuccess())
        {
            return AZ::Failure(VisitorError(VisitorErrorCode::InternalError, writeResult.TakeError()));
        }

        return AZ::Success();
    }

    Visitor::Result Backend::WriteToString(AZStd::string& buffer, WriteCallback callback)
    {
        AZ::IO::ByteContainerStream<AZStd::string> stream{&buffer};
        AZStd::unique_ptr<Visitor> writer = CreateStreamWriter(&stream);
        return callback(writer.get());
    }
}
