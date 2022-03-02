/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RenderDebugFeatureProcessor.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/span.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ {
    namespace Render {

        RenderDebugFeatureProcessor::RenderDebugFeatureProcessor()
        {
        }

        void RenderDebugFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<RenderDebugFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void RenderDebugFeatureProcessor::Activate()
        {
        }

        void RenderDebugFeatureProcessor::Deactivate()
        {
        }

        void RenderDebugFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "RenderDebugFeatureProcessor: Simulate");
            AZ_UNUSED(packet);
        }

        void RenderDebugFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "RenderDebugFeatureProcessor: Render");
            AZ_UNUSED(packet);
        }

    } // namespace Render
} // namespace AZ
