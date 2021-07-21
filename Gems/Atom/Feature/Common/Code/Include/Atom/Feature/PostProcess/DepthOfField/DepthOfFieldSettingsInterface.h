/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>

namespace AZ
{
    namespace Render
    {
        class DepthOfFieldSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::DepthOfFieldSettingsInterface, "{AC6DF992-DE82-4DF6-B071-75E81EFF5A65}");

            // Auto-gen virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
