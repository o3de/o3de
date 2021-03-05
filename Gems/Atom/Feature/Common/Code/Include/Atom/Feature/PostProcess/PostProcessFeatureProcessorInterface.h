/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        };
    }
}
