/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>

namespace AZ
{
    namespace Render
    {
        class AoRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::AoRequests, "{6F518EA6-D912-44C0-A79E-ABC16A30D2F7}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~AoRequests() {}

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<AoRequests> AoRequestBus;
    }
}
