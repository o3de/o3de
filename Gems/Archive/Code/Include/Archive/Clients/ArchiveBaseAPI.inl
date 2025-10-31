/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ArchiveInterfaceStructs.h"

namespace Archive
{
    constexpr bool AddCompressionAlgorithmId(Compression::CompressionAlgorithmId compressionAlgorithmId,
        ArchiveHeader& archiveHeader)
    {
        // The Invalid compression Algorithm Id is never added to the archive header compression algorithm id array
        // The uncompressed algorithm Id is not directly in the compression algorithm id array, but is
        // represented by the special index value of `0b111=7`
        if (compressionAlgorithmId == Compression::Invalid || compressionAlgorithmId == Compression::Uncompressed)
        {
            return false;
        }

        // Check if the compression algorithm id is already registered with the compression algorithm id array
        // If not, then register the compression algorithm at the first unused slot index
        size_t firstUnusedIndex = InvalidAlgorithmIndex;
        for (size_t compressionAlgorithmIndex{}; compressionAlgorithmIndex < archiveHeader.m_compressionAlgorithmsIds.size();
            ++compressionAlgorithmIndex)
        {
            Compression::CompressionAlgorithmId registeredCompressionAlgorithmId = archiveHeader.m_compressionAlgorithmsIds[compressionAlgorithmIndex];
            // If the compression algorithm is already registered return false
            if (registeredCompressionAlgorithmId == compressionAlgorithmId)
            {
                return false;
            }

            // track the firstUnused slot index when iterating over compression algorithm id array
            if (firstUnusedIndex == InvalidAlgorithmIndex
                && registeredCompressionAlgorithmId == Compression::Invalid)
            {
                firstUnusedIndex = compressionAlgorithmIndex;
            }
        }

        // If all the compression algorithm id indices are in use
        // no additional compression algorithms can be registered with the compression algorithm id array
        if (firstUnusedIndex == InvalidAlgorithmIndex)
        {
            return false;
        }

        // Insert the new compressionAlgorithmId into the first unused index of the compression algorithm id array
        archiveHeader.m_compressionAlgorithmsIds[firstUnusedIndex] = compressionAlgorithmId;
        return true;
    }

    constexpr size_t FindCompressionAlgorithmId(Compression::CompressionAlgorithmId compressionAlgorithmId,
        const ArchiveHeader& archiveHeader)
    {
        // If the compression algorithm id is invalid return the invalid algorithm index
        if (compressionAlgorithmId == Compression::Invalid)
        {
            return InvalidAlgorithmIndex;
        }

        // If the compression algorithm id is uncompressed
        // return a value of 7 which is the highest 3-bit value(0b111)
        // The compression algorithm Id array contains 7 elements for registering compression algorithms.
        // The index of 7 is used to represent that the file is uncompressed for th  purpose of storing
        // that information the archive TOC for an uncompressed file
        if (compressionAlgorithmId == Compression::Uncompressed)
        {
            return UncompressedAlgorithmIndex;
        }

        // Locate the compression algorithm Id in the compression algorithm id array
        auto foundIt = AZStd::find(archiveHeader.m_compressionAlgorithmsIds.begin(), archiveHeader.m_compressionAlgorithmsIds.end(),
            compressionAlgorithmId);
        return foundIt != archiveHeader.m_compressionAlgorithmsIds.end()
            ? AZStd::distance(archiveHeader.m_compressionAlgorithmsIds.begin(), foundIt)
            : InvalidAlgorithmIndex;
    }
} // namespace Archive
