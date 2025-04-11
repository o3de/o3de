/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <CompressionModuleInterface.h>
#include <Compression/CompressionTypeIds.h>

namespace Compression
{
    class CompressionModule
        : public CompressionModuleInterface
    {
    public:
        AZ_RTTI(CompressionModule, CompressionModuleTypeId, CompressionModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionModule, AZ::SystemAllocator);

        CompressionModule() = default;
        ~CompressionModule() = default;
    };
}// namespace Compression

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Compression::CompressionModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Compression, Compression::CompressionModule)
#endif
