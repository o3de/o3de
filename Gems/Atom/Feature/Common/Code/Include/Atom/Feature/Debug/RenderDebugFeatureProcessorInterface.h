/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ {
    namespace Render {

        //! TODO
        class RenderDebugFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::RenderDebugFeatureProcessorInterface, "{9774C763-178C-4CE2-99CD-3ABDE12445A4}", RPI::FeatureProcessor);

        };

    } // namespace Render
} // namespace AZ
