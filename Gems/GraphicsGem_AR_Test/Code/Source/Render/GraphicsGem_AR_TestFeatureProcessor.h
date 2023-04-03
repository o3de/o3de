/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestFeatureProcessorInterface.h>

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_TestFeatureProcessor final
        : public GraphicsGem_AR_TestFeatureProcessorInterface
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestFeatureProcessor, "{1F2AE769-FD53-4BC4-8C98-F8935455B4BE}", GraphicsGem_AR_TestFeatureProcessorInterface);
        AZ_CLASS_ALLOCATOR(GraphicsGem_AR_TestFeatureProcessor, AZ::SystemAllocator)

        static void Reflect(AZ::ReflectContext* context);

        GraphicsGem_AR_TestFeatureProcessor() = default;
        virtual ~GraphicsGem_AR_TestFeatureProcessor() = default;

        // FeatureProcessor overrides
        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

    };
}