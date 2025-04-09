/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <${Name}/${Name}FeatureProcessorInterface.h>

namespace ${Name}
{
    class ${Name}FeatureProcessor final
        : public ${Name}FeatureProcessorInterface
    {
    public:
        AZ_RTTI(${Name}FeatureProcessor, "{${Random_Uuid}}", ${Name}FeatureProcessorInterface);
        AZ_CLASS_ALLOCATOR(${Name}FeatureProcessor, AZ::SystemAllocator)

        static void Reflect(AZ::ReflectContext* context);

        ${Name}FeatureProcessor() = default;
        virtual ~${Name}FeatureProcessor() = default;

        // FeatureProcessor overrides
        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

    };
}
