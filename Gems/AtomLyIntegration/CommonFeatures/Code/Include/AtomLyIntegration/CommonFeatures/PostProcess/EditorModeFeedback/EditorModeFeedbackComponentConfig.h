/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        class EditorModeFeedbackComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeedbackComponentConfig, "{3DFFE0C6-118F-4B10-81F7-320BE5E4594B}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsTo(EditorModeFeedbackSettingsInterface* settings);
        };
    } // namespace Render
} // namespace AZ
