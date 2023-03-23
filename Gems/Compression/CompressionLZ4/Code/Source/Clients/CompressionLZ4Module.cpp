/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CompressionLZ4/CompressionLZ4TypeIds.h>
#include <CompressionLZ4ModuleInterface.h>
#include "CompressionLZ4SystemComponent.h"

namespace CompressionLZ4
{
    class CompressionLZ4Module
        : public CompressionLZ4ModuleInterface
    {
    public:
        AZ_RTTI(CompressionLZ4Module, CompressionLZ4ModuleTypeId, CompressionLZ4ModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionLZ4Module, AZ::SystemAllocator);
    };
}// namespace CompressionLZ4

AZ_DECLARE_MODULE_CLASS(Gem_CompressionLZ4, CompressionLZ4::CompressionLZ4Module)
