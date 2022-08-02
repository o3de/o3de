/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ONNXModuleInterface.h>
#include "ONNXSystemComponent.h"

namespace ONNX
{
    class ONNXModule
        : public ONNXModuleInterface
    {
    public:
        AZ_RTTI(ONNXModule, "{E006F52B-8EC8-4DFE-AB9D-C5EF7A1A8F32}", ONNXModuleInterface);
        AZ_CLASS_ALLOCATOR(ONNXModule, AZ::SystemAllocator, 0);
    };
}// namespace ONNX

AZ_DECLARE_MODULE_CLASS(Gem_ONNX, ONNX::ONNXModule)
