/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>

namespace AZ
{
    namespace Render
    {
        class DepthOfFieldRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::DepthOfFieldRequests, "{E00A8956-5960-4059-A5E8-55C040A65514}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~DepthOfFieldRequests() {}

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<DepthOfFieldRequests> DepthOfFieldRequestBus;
    }
}
