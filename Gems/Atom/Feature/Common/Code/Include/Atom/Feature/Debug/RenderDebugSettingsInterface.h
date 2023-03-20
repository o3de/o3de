/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/Debug/RenderDebugConstants.h>

namespace AZ::Render
{
    class RenderDebugSettingsInterface
    {
    public:
        AZ_RTTI(AZ::Render::RenderDebugSettingsInterface, "{03E799EC-E682-4C09-BE62-B8055B5FD936}");

        // Auto-gen virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

    };

}
