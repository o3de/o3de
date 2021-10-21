/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/PostProcess/EditorModeFeedback/EditorModeFeedbackComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorModeFeedbackComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorModeFeedbackComponentConfig, ComponentConfig>()->Version(0)

                // Auto-gen serialize context code...
#define SERIALIZE_CLASS EditorModeFeedbackComponentConfig
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void EditorModeFeedbackComponentConfig::CopySettingsTo (EditorModeFeedbackSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }
    } // namespace Render
} // namespace AZ
