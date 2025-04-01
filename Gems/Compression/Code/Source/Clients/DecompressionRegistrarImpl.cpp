/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DecompressionRegistrarImpl.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

#include <Compression/CompressionTypeIds.h>

namespace Compression
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(DecompressionRegistrarImpl, "DecompressionRegistrarImpl",
        DecompressionRegistrarImplTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(DecompressionRegistrarImpl, DecompressionRegistrarInterface);
    AZ_CLASS_ALLOCATOR_IMPL(DecompressionRegistrarImpl, AZ::SystemAllocator);

    DecompressionRegistrarImpl::DecompressionInterfaceDeleter::DecompressionInterfaceDeleter() = default;
    DecompressionRegistrarImpl::DecompressionInterfaceDeleter::DecompressionInterfaceDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}

    void DecompressionRegistrarImpl::DecompressionInterfaceDeleter::operator()(IDecompressionInterface* ptr) const
    {
        if (m_delete)
        {
            delete ptr;
        }
    }

    DecompressionRegistrarImpl::DecompressionRegistrarImpl() = default;
    DecompressionRegistrarImpl::~DecompressionRegistrarImpl() = default;

    void DecompressionRegistrarImpl::VisitDecompressionInterfaces(const VisitDecompressionInterfaceCallback& callback) const
    {
        auto VisitInterface = [&callback](const DecompressionIdIndexEntry& decompressionInterfaceEntry)
        {
            return decompressionInterfaceEntry.m_decompressionInterface != nullptr ? callback(*decompressionInterfaceEntry.m_decompressionInterface) : true;
        };

        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);
        AZStd::ranges::all_of(m_decompressionInterfaces, VisitInterface);
    }

    AZ::Outcome<void, AZStd::unique_ptr<IDecompressionInterface>> DecompressionRegistrarImpl::RegisterDecompressionInterface(
        CompressionAlgorithmId compressionAlgorithmId, AZStd::unique_ptr<IDecompressionInterface> decompressionInterface)
    {
        // Transfer ownership to a temporary DecompressionInterfacePtr which is supplied to the RegisterDecompressionInterfaceImpl
        // If registration fails, the compression interface pointer is returned in the failure outcome
        if (auto registerResult = RegisterDecompressionInterfaceImpl(compressionAlgorithmId, DecompressionInterfacePtr{ decompressionInterface.release() });
            !registerResult.IsSuccess())
        {
            return AZ::Failure(AZStd::unique_ptr<IDecompressionInterface>(registerResult.TakeError().release()));
        }

        // registration succeeded, return a void success outcome
        return AZ::Success();
    }

    bool DecompressionRegistrarImpl::RegisterDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId,
        IDecompressionInterface& decompressionInterface)
    {
        // Create a temporary DecompressionInterfacePtr with a custom deleter that does NOT delete the decompressionInterface reference
        // On success, the the DecompressionInterfacePtr is stored in the decompression interface array
        // and will not delete the reference to registered interface
        return RegisterDecompressionInterfaceImpl(compressionAlgorithmId, DecompressionInterfacePtr{ &decompressionInterface, DecompressionInterfaceDeleter{false} }).IsSuccess();
    }

    auto DecompressionRegistrarImpl::RegisterDecompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId,
        DecompressionInterfacePtr decompressionInterfacePtr)
        -> AZ::Outcome<void, DecompressionInterfacePtr>
    {
        if (decompressionInterfacePtr == nullptr)
        {
            return AZ::Failure(AZStd::move(decompressionInterfacePtr));
        }

        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);
        auto decompressionIter = FindDecompressionInterfaceImpl(compressionAlgorithmId);
        if (decompressionIter != m_decompressionInterfaces.end())
        {
            // The compression interface has been found using the compression interface id,
            // so another registration cannot be done
            return AZ::Failure(AZStd::move(decompressionInterfacePtr));
        }

        // Use UpperBound to find the insertion slot for the new entry
        auto ProjectionToCompressionAlgorithmId = [](const DecompressionIdIndexEntry& entry) -> CompressionAlgorithmId
        {
            return entry.m_id;
        };

        m_decompressionInterfaces.emplace(AZStd::ranges::upper_bound(m_decompressionInterfaces, compressionAlgorithmId,
            AZStd::ranges::less{}, ProjectionToCompressionAlgorithmId),
            DecompressionIdIndexEntry{ compressionAlgorithmId, AZStd::move(decompressionInterfacePtr) });

        return AZ::Success();
    }

    bool DecompressionRegistrarImpl::UnregisterDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId)
    {
        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);
        if (auto decompressionIter = FindDecompressionInterfaceImpl(compressionAlgorithmId);
            decompressionIter != m_decompressionInterfaces.end())
        {
            // Remove the decompressionInterface
            m_decompressionInterfaces.erase(decompressionIter);
            return true;
        }

        return false;
    }

    IDecompressionInterface* DecompressionRegistrarImpl::FindDecompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const
    {
        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);
        auto decompressionIter = FindDecompressionInterfaceImpl(compressionAlgorithmId);
        return decompressionIter != m_decompressionInterfaces.end() ? decompressionIter->m_decompressionInterface.get() : nullptr;
    }

    IDecompressionInterface* DecompressionRegistrarImpl::FindDecompressionInterface(AZStd::string_view algorithmName) const
    {
        // Potentially the entire vector is iterated over
        IDecompressionInterface* resultInterface{};
        auto FindCompressionInterfaceByName = [algorithmName, &resultInterface](const IDecompressionInterface& decompressionInterface)
        {
            if (decompressionInterface.GetCompressionAlgorithmName() == algorithmName)
            {
                // const_cast is being used here to avoid making a second visitor overload
                // NOTE: this is function is internal to the implementation of the Decompression Registrar
                resultInterface = const_cast<IDecompressionInterface*>(&decompressionInterface);
                // The matching interface has been found, halt visitation.
                return false;
            }

            return true;
        };
        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);

        VisitDecompressionInterfaces(FindCompressionInterfaceByName);
        return resultInterface;
    }

    bool DecompressionRegistrarImpl::IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const
    {
        AZStd::scoped_lock lock(m_decompressionInterfaceMutex);
        auto decompressionIter = FindDecompressionInterfaceImpl(compressionAlgorithmId);
        return decompressionIter != m_decompressionInterfaces.end();
    }

    // NOTE: The caller should lock the mutex
    // returns iterator to the compression inteface with the specified compression algorithm id
    // otherwise a sentinel iterator is returned
    auto DecompressionRegistrarImpl::FindDecompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId) const
        -> typename IdToDecompressionInterfaceMap::const_iterator
    {
        auto ProjectionToDecompressionAlgorithmId = [](const DecompressionIdIndexEntry& entry) -> CompressionAlgorithmId
        {
            return entry.m_id;
        };

        auto [firstFoundIter, lastFoundIter] = AZStd::ranges::equal_range(m_decompressionInterfaces,
            compressionAlgorithmId, AZStd::ranges::less{}, ProjectionToDecompressionAlgorithmId);

        return firstFoundIter != lastFoundIter ? firstFoundIter : m_decompressionInterfaces.end();
    }
}// namespace Compression
