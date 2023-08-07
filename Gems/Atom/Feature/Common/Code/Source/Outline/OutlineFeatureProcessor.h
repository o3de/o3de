/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class OutlineFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(OutlineFeatureProcessor, AZ::SystemAllocator)

        AZ_RTTI(AZ::Render::OutlineFeatureProcessor, "{E32ABBE6-2472-4404-AEDB-1CE7A12E7C43}", AZ::RPI::FeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        OutlineFeatureProcessor();
        ~OutlineFeatureProcessor() override;

        //! FeatureProcessor 
        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline) override;
    };
}

