/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionFactoryImpl.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace Compression
{
    CompressionFactoryImpl::~CompressionFactoryImpl() = default;

    void CompressionFactoryImpl::VisitCompressionInterfaces(const VisitCompressionInterfaceCallback& callback) const
    {
        auto VisitInterface = [&callback](const AZStd::unique_ptr<ICompressionInterface>& compressionInterface)
        {
            return compressionInterface != nullptr ? callback(*compressionInterface) : true;
        };

        AZStd::ranges::all_of(m_compressionInterfaces, VisitInterface);
    }

    bool CompressionFactoryImpl::RegisterCompressionInterface(AZStd::unique_ptr<ICompressionInterface>&& compressionInterface)
    {
        if (compressionInterface == nullptr)
        {
            return false;
        }

        CompressionAlgorithmId compressionAlgorithmId = compressionInterface->GetCompressionAlgorithmId();
        const size_t compressionIndex = FindCompressionIndex(compressionAlgorithmId);
        if (compressionIndex < m_compressionInterfaces.size())
        {
            return false;
        }

        // Insert new compression interface since it is not registered
        m_compressionInterfaces.emplace_back(AZStd::move(compressionInterface));
        const size_t emplaceIndex = m_compressionInterfaces.size() - 1;

        // Use UpperBound to find the insertion slot for the new entry within the compression index set
        auto FindIdIndexEntry = [](const CompressionIdIndexEntry& lhs, const CompressionIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };
        CompressionIdIndexEntry newEntry{ compressionAlgorithmId, emplaceIndex };
        m_compressionIdIndexSet.insert(AZStd::upper_bound(m_compressionIdIndexSet.begin(), m_compressionIdIndexSet.end(),
            newEntry, AZStd::move(FindIdIndexEntry)),
            AZStd::move(newEntry));
        return true;
    }

    bool CompressionFactoryImpl::UnregisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId)
    {
        const size_t compressionIndex = FindCompressionIndex(compressionAlgorithmId);
        if (compressionIndex < m_compressionInterfaces.size())
        {
            auto oldInterfaceIter = m_compressionInterfaces.begin() + compressionIndex;
            m_compressionInterfaces.erase(oldInterfaceIter);
            return true;
        }

        return false;
    }

    ICompressionInterface* CompressionFactoryImpl::FindCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const
    {
        const size_t compressionIndex = FindCompressionIndex(compressionAlgorithmId);
        return compressionIndex < m_compressionInterfaces.size()
            ? m_compressionInterfaces[compressionIndex].get() : nullptr;
    }

    // find compression index entry in vector
    // returns the index into the m_compressionInterfaces vector that matches the compression algorithm Id
    // otherwise returns the size of the m_compressionInterfaces container to indicate out of bounds
    size_t CompressionFactoryImpl::FindCompressionIndex(CompressionAlgorithmId compressionAlgorithmId) const
    {
        auto FindIdIndexEntry = [](const CompressionIdIndexEntry& lhs, const CompressionIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };
        auto searchIter = AZStd::lower_bound(m_compressionIdIndexSet.begin(), m_compressionIdIndexSet.end(),
            CompressionIdIndexEntry{ compressionAlgorithmId }, FindIdIndexEntry);

        return searchIter != m_compressionIdIndexSet.end() && !FindIdIndexEntry(
            CompressionIdIndexEntry{ compressionAlgorithmId }, *searchIter) ?
            searchIter->m_index : m_compressionInterfaces.size();
    }
}// namespace Compression
