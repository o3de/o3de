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
#include <Compression/DecompressionInterfaceAPI.h>

namespace Compression
{
    class DecompressionRegistrarImpl final
        : public DecompressionRegistrarInterface
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(DecompressionRegistrarImpl);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        DecompressionRegistrarImpl();
        ~DecompressionRegistrarImpl();

        void VisitDecompressionInterfaces(const VisitDecompressionInterfaceCallback&) const override;

        //! Registers a decompression interface with a standard deleter
        AZ::Outcome<void, AZStd::unique_ptr<IDecompressionInterface>> RegisterDecompressionInterface(
            CompressionAlgorithmId algorithmId,
            AZStd::unique_ptr<IDecompressionInterface> decompressionInterface) override;

        //! Registers a decompression interface with a null deleter
        bool RegisterDecompressionInterface(CompressionAlgorithmId algorithmId, IDecompressionInterface& decompressionInterface) override;

        bool UnregisterDecompressionInterface(CompressionAlgorithmId algorithmId) override;

        [[nodiscard]] IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId algorithmId) const override;
        [[nodiscard]] IDecompressionInterface* FindDecompressionInterface(AZStd::string_view algorithmName) const override;

        [[nodiscard]] bool IsRegistered(CompressionAlgorithmId algorithmId) const override;

        struct DecompressionInterfaceDeleter
        {
            DecompressionInterfaceDeleter();
            DecompressionInterfaceDeleter(bool shouldDelete);
            void operator()(IDecompressionInterface* ptr) const;
            bool m_delete{ true };
        };
    private:
        using DecompressionInterfacePtr = AZStd::unique_ptr<IDecompressionInterface, DecompressionInterfaceDeleter>;

        //! Helper function that is used to register a decompression interface into the decompression interface array
        //! while taking into account whether the decompression interface should be owned by this registrar
        //! @param compressionAlgorithmId Unique Id of the decompression interface to register with this registrar
        //! @param decompressionInterface unique_ptr to decompression interface to register
        //! @return outcome which indicates whether the decompression interface was registered with the decompression interface array
        //! On success an empty Success outcome is returned
        //! On failure, the supplied decompression interface parameter is returned back to the caller
        [[nodiscard]] AZ::Outcome<void, DecompressionInterfacePtr> RegisterDecompressionInterfaceImpl(
            CompressionAlgorithmId algorithmId, DecompressionInterfacePtr decompressionInterface);

        struct DecompressionIdIndexEntry
        {
            CompressionAlgorithmId m_id;
            DecompressionInterfacePtr m_decompressionInterface;
        };

        using IdToDecompressionInterfaceMap = AZStd::vector<DecompressionIdIndexEntry>;

        //! Searches within the decompression interface array for the decompression interface registered with the specified id
        //! @param compressionAlgorithmId Unique Id of decompression interface to locate
        //! @return iterator pointing the decompression interface registered with the specified CompressionAlgorithmId
        //! NOTE: It is responsibility of the caller to lock the decompression interface mutex to protect the search
        typename IdToDecompressionInterfaceMap::const_iterator FindDecompressionInterfaceImpl(CompressionAlgorithmId compressionAlgorithmId) const;


        //! Contains the registered decompression interfaces
        //! Sorted to provide O(Log N) search
        IdToDecompressionInterfaceMap m_decompressionInterfaces;

        //! Protects modifications to the decompression interfaces container
        mutable AZStd::mutex m_decompressionInterfaceMutex;
    };
}// namespace Compression
