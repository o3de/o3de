/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>
#include <AzCore/IO/GenericStreams.h>

class GenericStreamMock : public AZ::IO::GenericStream
{
public:

    MOCK_CONST_METHOD0(IsOpen, bool());
    MOCK_CONST_METHOD0(CanSeek, bool());
    MOCK_CONST_METHOD0(CanRead, bool());
    MOCK_CONST_METHOD0(CanWrite, bool());
    MOCK_METHOD2(Seek, void(AZ::IO::OffsetType, SeekMode));
    MOCK_METHOD2(Read, AZ::IO::SizeType(AZ::IO::SizeType, void*));
    MOCK_METHOD2(Write, AZ::IO::SizeType(AZ::IO::SizeType, const void*));
    MOCK_METHOD2(WriteFromStream, AZ::IO::SizeType(AZ::IO::SizeType, GenericStream*));
    MOCK_CONST_METHOD0(GetCurPos, AZ::IO::SizeType());
    MOCK_CONST_METHOD0(GetLength, AZ::IO::SizeType());
    MOCK_METHOD3(ReadAtOffset, AZ::IO::SizeType(AZ::IO::SizeType, void*, AZ::IO::OffsetType));
    MOCK_METHOD3(WriteAtOffset, AZ::IO::SizeType(AZ::IO::SizeType, const void*, AZ::IO::OffsetType));
    MOCK_CONST_METHOD0(IsCompressed, bool());
    MOCK_CONST_METHOD0(GetFilename, const char* ());
    MOCK_CONST_METHOD0(GetModeFlags, AZ::IO::OpenMode());
    MOCK_METHOD0(ReOpen, bool());
    MOCK_METHOD0(Close, void());

    // Provide methods that call the concrete implementations for all the non-pure-virtual methods in the interface

    AZ::IO::SizeType BaseWriteFromStream(AZ::IO::SizeType bytes, GenericStream* inputStream)
    {
        return AZ::IO::GenericStream::WriteFromStream(bytes, inputStream);
    }

    AZ::IO::SizeType BaseReadAtOffset(AZ::IO::SizeType bytes, void* oBuffer, AZ::IO::OffsetType offset = -1)
    {
        return AZ::IO::GenericStream::ReadAtOffset(bytes, oBuffer, offset);
    }

    AZ::IO::SizeType BaseWriteAtOffset(AZ::IO::SizeType bytes, const void* iBuffer, AZ::IO::OffsetType offset = -1)
    {
        return AZ::IO::GenericStream::WriteAtOffset(bytes, iBuffer, offset);
    }

    bool BaseIsCompressed() const { return AZ::IO::GenericStream::IsCompressed(); }
    const char* BaseGetFilename() const { return AZ::IO::GenericStream::GetFilename(); }
    AZ::IO::OpenMode BaseGetModeFlags() const { return AZ::IO::GenericStream::GetModeFlags(); }
    bool BaseReOpen() { return AZ::IO::GenericStream::ReOpen(); }
    void BaseClose() { AZ::IO::GenericStream::Close(); }
};
