/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

namespace AZ
{
    namespace Render
    {
        class EditorModeFeedbackSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeedbackSettingsInterface, "{ED6C322E-90C5-45C0-AC2E-3F20CF1CB5BA}");

            // Auto-gen virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };
    }
}
