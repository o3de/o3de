/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>

namespace AZ {
    namespace Render {

        //! TODO
        class RenderDebugFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::RenderDebugFeatureProcessorInterface, "{9774C763-178C-4CE2-99CD-3ABDE12445A4}", RPI::FeatureProcessor);

            //! Retrieves existing settings. If none found, returns nullptr.
            virtual RenderDebugSettingsInterface* GetSettingsInterface() = 0;

            //! Retrieves existing settings. If none found, creates a new instance.
            virtual RenderDebugSettingsInterface* GetOrCreateSettingsInterface() = 0;

            //! Removes settings for corresponding entity ID.
            virtual void RemoveSettingsInterface() = 0;

            //! Called to notify the feature processor that changes have been made to it's owned post process settings
            virtual void OnPostProcessSettingsChanged() = 0;
        };

    } // namespace Render
} // namespace AZ
