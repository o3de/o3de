/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Compression/CompressionInterfaceAPI.h>

namespace Compression
{
    class CompressionFactoryImpl final
        : public CompressionFactoryInterface
    {
    public:
        AZ_RTTI(CompressionFactoryImpl, "{9F3B8418-4BEB-4249-BAAF-6653A8F511A3}", CompressionFactoryInterface);
        AZ_CLASS_ALLOCATOR(CompressionFactoryImpl, AZ::SystemAllocator, 0);

        ~CompressionFactoryImpl();

        void VisitCompressionInterfaces(const VisitCompressionInterfaceCallback&) const override;
        bool RegisterCompressionInterface(AZStd::unique_ptr<ICompressionInterface>&&) override;
        bool UnregisterCompressionInterface(CompressionAlgorithmId) override;
        ICompressionInterface* FindCompressionInterface(CompressionAlgorithmId) const override;

    private:
        size_t FindCompressionIndex(CompressionAlgorithmId) const;
        struct CompressionIdIndexEntry
        {
            CompressionAlgorithmId m_id;
            size_t m_index;
        };
        //! Index into the Compression Interfaces vector
        AZStd::vector<CompressionIdIndexEntry> m_compressionIdIndexSet;
        AZStd::vector<AZStd::unique_ptr<ICompressionInterface>> m_compressionInterfaces;
    };
}// namespace Compression
