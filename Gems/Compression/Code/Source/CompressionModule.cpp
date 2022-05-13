/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <CompressionModuleInterface.h>
#include <CompressionSystemComponent.h>

namespace Compression
{
    class CompressionModule
        : public CompressionModuleInterface
    {
    public:
        AZ_RTTI(CompressionModule, "{6D256D91-6F1F-4132-B78E-6C24BA9D688C}", CompressionModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionModule, AZ::SystemAllocator, 0);
    };
}// namespace Compression

AZ_DECLARE_MODULE_CLASS(Gem_Compression, Compression::CompressionModule)
