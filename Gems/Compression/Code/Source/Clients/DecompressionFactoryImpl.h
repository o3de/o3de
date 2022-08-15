/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Compression/DecompressionInterfaceAPI.h>

namespace Compression
{
    class DecompressionFactoryImpl final
        : public DecompressionFactoryInterface
    {
    public:
        AZ_RTTI(DecompressionFactoryImpl, "{2353362A-A059-4681-ADF0-5ABE41E85A6B}", DecompressionFactoryInterface);
        AZ_CLASS_ALLOCATOR(DecompressionFactoryImpl, AZ::SystemAllocator, 0);

        ~DecompressionFactoryImpl();

        void VisitDecompressionInterfaces(const VisitDecompressionInterfaceCallback&) const override;
        bool RegisterDecompressionInterface(AZStd::unique_ptr<IDecompressionInterface>&&) override;
        bool UnregisterDecompressionInterface(CompressionAlgorithmId) override;
        IDecompressionInterface* FindDecompressionInterface(CompressionAlgorithmId) const override;

    private:
        size_t FindDecompressionIndex(CompressionAlgorithmId) const;
        struct DecompressionIdIndexEntry
        {
            CompressionAlgorithmId m_id;
            size_t m_index;
        };
        //! Index into the Decompression Interfaces vector
        AZStd::vector<DecompressionIdIndexEntry> m_decompressionIdIndexSet;
        AZStd::vector<AZStd::unique_ptr<IDecompressionInterface>> m_decompressionInterfaces;
    };
}// namespace Compression
