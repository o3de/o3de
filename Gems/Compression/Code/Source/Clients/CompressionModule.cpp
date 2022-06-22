/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <CompressionModuleInterface.h>
#include "CompressionSystemComponent.h"
#include <Compression/DecompressionInterfaceAPI.h>

namespace Compression
{
    class CompressionModule
        : public CompressionModuleInterface
    {
    public:
        AZ_RTTI(CompressionModule, "{6D256D91-6F1F-4132-B78E-6C24BA9D688C}", CompressionModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionModule, AZ::SystemAllocator, 0);

         CompressionModule()
        {
            // Create and Register the Decompression Factory
            if (DecompressionFactory::Get() == nullptr)
            {
                DecompressionFactory::Register(m_decompressionFactoryInterface.get());
            }
        }

        ~CompressionModule()
        {
            if (DecompressionFactory::Get() == m_decompressionFactoryInterface.get())
            {
                DecompressionFactory::Unregister(m_decompressionFactoryInterface.get());
            }
        }
    private:
        // DecompressionFactory interface used to register Decompression interfaces
        // Available in ALL applications to allow decompression to occur
        AZStd::unique_ptr<DecompressionFactoryInterface> m_decompressionFactoryInterface;
    };
}// namespace Compression

AZ_DECLARE_MODULE_CLASS(Gem_Compression, Compression::CompressionModule)
