/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DecompressionFactoryImpl.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace Compression
{
    DecompressionFactoryImpl::~DecompressionFactoryImpl() = default;

    void DecompressionFactoryImpl::VisitDecompressionInterfaces(const VisitDecompressionInterfaceCallback& callback) const
    {
        auto VisitInterface = [&callback](const AZStd::unique_ptr<IDecompressionInterface>& decompressionInterface)
        {
            return decompressionInterface != nullptr ? callback(*decompressionInterface) : true;
        };

        AZStd::ranges::all_of(m_decompressionInterfaces, VisitInterface);
    }

    bool DecompressionFactoryImpl::RegisterDecompressionInterface(AZStd::unique_ptr<IDecompressionInterface>&& decompressionInterface)
    {
        if (decompressionInterface == nullptr)
        {
            return false;
        }

        CompressionAlgorithmId compressionAlgorithmId = decompressionInterface->GetCompressionAlgorithmId();
        const size_t compressionIndex = FindDecompressionIndex(compressionAlgorithmId);
        if (compressionIndex < m_decompressionInterfaces.size())
        {
            return false;
        }

        // Insert new compression interface since it is not registered
        m_decompressionInterfaces.emplace_back(AZStd::move(decompressionInterface));
        const size_t emplaceIndex = m_decompressionInterfaces.size() - 1;

        // Use UpperBound to find the insertion slot for the new entry within the compression index set
        auto FindIdIndexEntry = [](const DecompressionIdIndexEntry& lhs, const DecompressionIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };
        DecompressionIdIndexEntry newEntry{ compressionAlgorithmId, emplaceIndex };
        m_decompressionIdIndexSet.insert(AZStd::upper_bound(m_decompressionIdIndexSet.begin(), m_decompressionIdIndexSet.end(),
            newEntry, AZStd::move(FindIdIndexEntry)),
            AZStd::move(newEntry));
        return true;
    }

    bool DecompressionFactoryImpl::UnregisterDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId)
    {
        const size_t compressionIndex = FindDecompressionIndex(compressionAlgorithmId);
        if (compressionIndex < m_decompressionInterfaces.size())
        {
            auto oldInterfaceIter = m_decompressionInterfaces.begin() + compressionIndex;
            m_decompressionInterfaces.erase(oldInterfaceIter);
            return true;
        }

        return false;
    }

    IDecompressionInterface* DecompressionFactoryImpl::FindDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const
    {
        const size_t compressionIndex = FindDecompressionIndex(compressionAlgorithmId);
        return compressionIndex < m_decompressionInterfaces.size()
            ? m_decompressionInterfaces[compressionIndex].get() : nullptr;
    }

    // find decompression index entry in vector
    // returns the index into the m_decompressionInterfaces vector that matches the compression algorithm Id
    // otherwise returns the size of the m_decompressionInterfaces container to indicate out of bounds
    size_t DecompressionFactoryImpl::FindDecompressionIndex(CompressionAlgorithmId compressionAlgorithmId) const
    {
        auto FindIdIndexEntry = [](const DecompressionIdIndexEntry& lhs, const DecompressionIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };
        auto searchIter = AZStd::lower_bound(m_decompressionIdIndexSet.begin(), m_decompressionIdIndexSet.end(),
            DecompressionIdIndexEntry{ compressionAlgorithmId }, FindIdIndexEntry);

        return searchIter != m_decompressionIdIndexSet.end() && !FindIdIndexEntry(
            DecompressionIdIndexEntry{ compressionAlgorithmId }, *searchIter) ?
            searchIter->m_index : m_decompressionInterfaces.size();
    }
}// namespace Compression
