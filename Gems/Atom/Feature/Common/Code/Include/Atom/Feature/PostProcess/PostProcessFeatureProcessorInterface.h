/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        //! Abstract interface for PostProcessFeatureProcessor so it can be access outside of Atom (for example in AtomLyIntegration)
        class PostProcessFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::PostProcessFeatureProcessorInterface, "{720EE4B9-78C1-4151-AFEF-13DB5CC500D1}", AZ::RPI::FeatureProcessor);

            //! Retrieves existing post process settings for corresponding entity ID. If none found, returns nullptr.
            virtual PostProcessSettingsInterface* GetSettingsInterface(EntityId entityId) = 0;

            //! Retrieves existing post process settings for corresponding entity ID. If none found, creates a new instance.
            virtual PostProcessSettingsInterface* GetOrCreateSettingsInterface(EntityId entityId) = 0;

            //! Removes post process settings for corresponding entity ID.
            virtual void RemoveSettingsInterface(EntityId entityId) = 0;

            //! Called to notify the feature processor that changes have been made to it's owned post process settings
            virtual void OnPostProcessSettingsChanged() = 0;

            //! Set an alias for the given sourceView
            virtual void SetViewAlias(const AZ::RPI::ViewPtr sourceView, const AZ::RPI::ViewPtr targetView) = 0;

            //! Removes the alias for sourceView
            virtual void RemoveViewAlias(const AZ::RPI::ViewPtr sourceView) = 0;
        };
    }
}
