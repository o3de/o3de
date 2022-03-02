/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <Atom/Feature/Debug/RenderDebugFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include "Atom/RPI.Public/Image/StreamingImage.h"
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
    }

    namespace Render
    {
        class RenderDebugFeatureProcessor final
            : public AZ::Render::RenderDebugFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::RenderDebugFeatureProcessor, "{1F14912D-43E1-4992-9822-BE8967E59EA3}", AZ::Render::RenderDebugFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            RenderDebugFeatureProcessor();
            virtual ~RenderDebugFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const RPI::FeatureProcessor::SimulatePacket& packet) override;
            void Render(const RPI::FeatureProcessor::RenderPacket& packet) override;

        private:

            RenderDebugFeatureProcessor(const RenderDebugFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "RenderDebugFeatureProcessor";

        };

    } // namespace Render
} // namespace AZ
