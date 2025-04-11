/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Compression/CompressionInterfaceAPI.h>

namespace Compression
{
    class CompressionRegistrarImpl final
        : public CompressionRegistrarInterface
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(CompressionRegistrarImpl);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        CompressionRegistrarImpl();
        ~CompressionRegistrarImpl();

        void VisitCompressionInterfaces(const VisitCompressionInterfaceCallback&) const override;

        //! Registers a compression interface with a standard deleter
        AZ::Outcome<void, AZStd::unique_ptr<ICompressionInterface>> RegisterCompressionInterface(
            CompressionAlgorithmId algorithmId,
            AZStd::unique_ptr<ICompressionInterface> compressionInterface) override;

        //! Registers a compression interface with a null deleter
        bool RegisterCompressionInterface(CompressionAlgorithmId algorithmId, ICompressionInterface& compressionInterface) override;

        bool UnregisterCompressionInterface(CompressionAlgorithmId algorithmId) override;

        [[nodiscard]] ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId algorithmId) const override;
        [[nodiscard]] ICompressionInterface* FindCompressionInterface(AZStd::string_view algorithmName) const override;

        [[nodiscard]] bool IsRegistered(CompressionAlgorithmId algorithmId) const override;

        struct CompressionInterfaceDeleter
        {
            CompressionInterfaceDeleter();
            CompressionInterfaceDeleter(bool shouldDelete);
            void operator()(ICompressionInterface* ptr) const;
            bool m_delete{ true };
        };
    private:
        using CompressionInterfacePtr = AZStd::unique_ptr<ICompressionInterface, CompressionInterfaceDeleter>;

        //! Helper function that is used to register a compression interface into the compression interface array
        //! while taking into account whether the compression interface should be owned by this registrar
        //! @param compressionAlgorithmId Unique Id of the compression interface to register with this registrar
        //! @param compression unique_ptr to compression interface to register
        //! @return outcome which indicates whether the compression interface was registered with the compression interface array
        //! On success an empty Success outcome is returned
        //! On failure, the supplied compression interface parameter is returned back to the caller
        [[nodiscard]] AZ::Outcome<void, CompressionInterfacePtr> RegisterCompressionInterfaceImpl(
            CompressionAlgorithmId algorithmId, CompressionInterfacePtr compressionInterface);

        struct CompressionIdIndexEntry
        {
            CompressionAlgorithmId m_id;
            CompressionInterfacePtr m_compressionInterface;
        };

        using IdToCompressionInterfaceMap = AZStd::vector<CompressionIdIndexEntry>;

        //! Searches within the compression interface array for the compression interface registered with the specified id
        //! @param compressionAlgorithmId Unique Id of compression interface to locate
        //! @return iterator pointing the compression interface registered with the specified CompressionAlgorithmId
        //! NOTE: It is responsibility of the caller to lock the compression interface mutex to protect the search
        typename IdToCompressionInterfaceMap::const_iterator FindCompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId) const;


        //! Contains the registered compression interfaces
        //! Sorted to provide O(Log N) search
        IdToCompressionInterfaceMap m_compressionInterfaces;

        //! Protects modifications to the compression interfaces container
        mutable AZStd::mutex m_compressionInterfaceMutex;
    };
}// namespace Compression
