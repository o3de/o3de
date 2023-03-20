/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/IO/Compressor.h>
#include <AzCore/IO/CompressorStream.h>

namespace AZ::IO
{
    //=========================================================================
    // WriteHeaderAndData
    // [12/13/2012]
    //=========================================================================
    bool Compressor::WriteHeaderAndData(CompressorStream* compressorStream)
    {
        AZ_Assert(compressorStream->CanWrite(), "Stream is not open for write!");
        AZ_Assert(compressorStream->GetCompressorData(), "Stream doesn't have attached compressor, call WriteCompressed first!");
        AZ_Assert(compressorStream->GetCompressorData()->m_compressor == this, "Invalid compressor data! Data belongs to a different compressor");
        CompressorHeader header;
        header.SetAZCS();
        header.m_compressorId = GetTypeId();
        header.m_uncompressedSize = compressorStream->GetCompressorData()->m_uncompressedSize;
        AZStd::endian_swap(header.m_compressorId);
        AZStd::endian_swap(header.m_uncompressedSize);
        GenericStream* baseStream = compressorStream->GetWrappedStream();
        if (baseStream->WriteAtOffset(sizeof(CompressorHeader), &header, 0U) == sizeof(CompressorHeader))
        {
            return true;
        }

        return false;
    }
} // namespace AZ::IO
