/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Asset/AssetCommon.h>


namespace LyShine
{
    class LyShineFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(LyShineFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(LyShineFeatureProcessor, "{D6218A9D-5F27-4ACC-8F89-CCBDAFD24693}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(LyShineFeatureProcessor);
        AZ_FEATURE_PROCESSOR(LyShineFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        LyShineFeatureProcessor() = default;
        ~LyShineFeatureProcessor() = default;

    private:        
        // AZ::RPI::FeatureProcessor overrides...
        void AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline) override;

        AZ::Data::Asset<AZ::RPI::AnyAsset> m_passRequestAsset;
    };
}
