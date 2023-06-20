/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionRegistrarImpl.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

#include <Compression/CompressionTypeIds.h>

namespace Compression
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(CompressionRegistrarImpl, "CompressionRegistrarImpl",
        CompressionRegistrarImplTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(CompressionRegistrarImpl, CompressionRegistrarInterface);
    AZ_CLASS_ALLOCATOR_IMPL(CompressionRegistrarImpl, AZ::SystemAllocator);

    CompressionRegistrarImpl::CompressionInterfaceDeleter::CompressionInterfaceDeleter() = default;
    CompressionRegistrarImpl::CompressionInterfaceDeleter::CompressionInterfaceDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}
    void CompressionRegistrarImpl::CompressionInterfaceDeleter::operator()(ICompressionInterface* ptr) const
    {
        if (m_delete)
        {
            delete ptr;
        }
    }

    CompressionRegistrarImpl::CompressionRegistrarImpl() = default;
    CompressionRegistrarImpl::~CompressionRegistrarImpl() = default;

    void CompressionRegistrarImpl::VisitCompressionInterfaces(const VisitCompressionInterfaceCallback& callback) const
    {
        auto VisitInterface = [&callback](const CompressionIdIndexEntry& compressionInterfaceEntry)
        {
            return compressionInterfaceEntry.m_compressionInterface != nullptr ? callback(*compressionInterfaceEntry.m_compressionInterface) : true;
        };

        AZStd::scoped_lock lock(m_compressionInterfaceMutex);
        AZStd::ranges::all_of(m_compressionInterfaces, VisitInterface);
    }

    AZ::Outcome<void, AZStd::unique_ptr<ICompressionInterface>> CompressionRegistrarImpl::RegisterCompressionInterface(
        CompressionAlgorithmId compressionAlgorithmId, AZStd::unique_ptr<ICompressionInterface> compressionInterface)
    {
        // Transfer ownership to a temporary CompressionInterfacePtr which is supplied to the RegisterCompressionInterfaceImpl
        // If registration fails, the compression interface pointer is returned in the failure outcome
        if (auto registerResult = RegisterCompressionInterfaceImpl(compressionAlgorithmId, CompressionInterfacePtr{ compressionInterface.release() });
            !registerResult.IsSuccess())
        {
            return AZ::Failure(AZStd::unique_ptr<ICompressionInterface>(registerResult.TakeError().release()));
        }

        // registration succeeded, return a void success outcome
        return AZ::Success();
    }

    bool CompressionRegistrarImpl::RegisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId, ICompressionInterface& compressionInterface)
    {
        // Create a temporary CompressionInterfacePtr with a custom deleter that does NOT delete the compressionInterface reference
        // On success, the the CompressionInterfacePtr is stored in the compression interface array
        // and will not delete the reference to registered interface
        return RegisterCompressionInterfaceImpl(compressionAlgorithmId, CompressionInterfacePtr{ &compressionInterface, CompressionInterfaceDeleter{false} }).IsSuccess();
    }

    auto CompressionRegistrarImpl::RegisterCompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId, CompressionInterfacePtr compressionInterfacePtr)
        -> AZ::Outcome<void, CompressionInterfacePtr>
    {
        if (compressionInterfacePtr == nullptr)
        {
            return AZ::Failure(AZStd::move(compressionInterfacePtr));
        }

        AZStd::scoped_lock lock(m_compressionInterfaceMutex);
        auto compressionIter = FindCompressionInterfaceImpl(compressionAlgorithmId);
        if (compressionIter != m_compressionInterfaces.end())
        {
            // The compression interface has been found using the compression interface id,
            // so another registration cannot be done
            return AZ::Failure(AZStd::move(compressionInterfacePtr));
        }

        // Use UpperBound to find the insertion slot for the new entry
        auto ProjectionToCompressionAlgorithmId = [](const CompressionIdIndexEntry& entry) -> CompressionAlgorithmId
        {
            return entry.m_id;
        };

        m_compressionInterfaces.emplace(AZStd::ranges::upper_bound(m_compressionInterfaces, compressionAlgorithmId,
            AZStd::ranges::less{}, ProjectionToCompressionAlgorithmId),
            CompressionIdIndexEntry{ compressionAlgorithmId, AZStd::move(compressionInterfacePtr) });

        return AZ::Success();
    }

    bool CompressionRegistrarImpl::UnregisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId)
    {
        AZStd::scoped_lock lock(m_compressionInterfaceMutex);
        if (auto compressionIter = FindCompressionInterfaceImpl(compressionAlgorithmId);
            compressionIter != m_compressionInterfaces.end())
        {
            // Remove the compressionInterface
            m_compressionInterfaces.erase(compressionIter);
            return true;
        }

        return false;
    }

    ICompressionInterface* CompressionRegistrarImpl::FindCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const
    {
        AZStd::scoped_lock lock(m_compressionInterfaceMutex);
        auto compressionIter = FindCompressionInterfaceImpl(compressionAlgorithmId);
        return compressionIter != m_compressionInterfaces.end() ? compressionIter->m_compressionInterface.get() : nullptr;
    }


    ICompressionInterface* CompressionRegistrarImpl::FindCompressionInterface(AZStd::string_view algorithmName) const
    {
        // Potentially the entire vector is iterated over
        ICompressionInterface* resultInterface{};
        auto FindCompressionInterfaceByName = [algorithmName, &resultInterface](const ICompressionInterface& compressionInterface)
        {
            if (compressionInterface.GetCompressionAlgorithmName() == algorithmName)
            {
                // const_cast is being used here to avoid making a second visitor overload
                // NOTE: this is function is internal to the implementation of the Compression Registrar
                resultInterface = const_cast<ICompressionInterface*>(&compressionInterface);
                // The matching interface has been found, halt visitation.
                return false;
            }

            return true;
        };
        AZStd::scoped_lock lock(m_compressionInterfaceMutex);

        VisitCompressionInterfaces(FindCompressionInterfaceByName);
        return resultInterface;
    }

    bool CompressionRegistrarImpl::IsRegistered(CompressionAlgorithmId compressionAlgorithmId) const
    {
        AZStd::scoped_lock lock(m_compressionInterfaceMutex);
        auto compressionIter = FindCompressionInterfaceImpl(compressionAlgorithmId);
        return compressionIter != m_compressionInterfaces.end();
    }

    // NOTE: The caller should lock the mutex
    // returns iterator to the compression inteface with the specified compression algorithm id
    // otherwise a sentinel iterator is returned
    auto CompressionRegistrarImpl::FindCompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId) const
        -> typename IdToCompressionInterfaceMap::const_iterator
    {
        auto ProjectionToCompressionAlgorithmId = [](const CompressionIdIndexEntry& entry) -> CompressionAlgorithmId
        {
            return entry.m_id;
        };

        auto [firstFoundIter, lastFoundIter] = AZStd::ranges::equal_range(m_compressionInterfaces,
            compressionAlgorithmId, AZStd::ranges::less{}, ProjectionToCompressionAlgorithmId);

        return firstFoundIter != lastFoundIter ? firstFoundIter : m_compressionInterfaces.end();
    }
}// namespace Compression
