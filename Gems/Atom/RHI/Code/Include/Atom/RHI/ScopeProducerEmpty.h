/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>

namespace AZ::RHI
{
    class ScopeProducerEmpty final
        : public ScopeProducer
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopeProducerEmpty, SystemAllocator);

        ScopeProducerEmpty(const ScopeId& scopeId, int deviceIndex = MultiDevice::InvalidDeviceIndex)
            : ScopeProducer(scopeId, deviceIndex)
        {}

    private:
        void SetupFrameGraphDependencies(FrameGraphInterface) override {}
        void CompileResources(const FrameGraphCompileContext&) override {}
        void BuildCommandList(const FrameGraphExecuteContext&) override {}
    };
}
