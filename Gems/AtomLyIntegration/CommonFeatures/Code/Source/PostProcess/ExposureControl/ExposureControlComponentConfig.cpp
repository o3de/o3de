/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void ExposureControlComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ExposureControlComponentConfig, ComponentConfig>()
                    ->Version(1)

                    // Auto-gen serialize context code...
#define SERIALIZE_CLASS ExposureControlComponentConfig
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void ExposureControlComponentConfig::CopySettingsFrom(ExposureControlSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void ExposureControlComponentConfig::CopySettingsTo(ExposureControlSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ
