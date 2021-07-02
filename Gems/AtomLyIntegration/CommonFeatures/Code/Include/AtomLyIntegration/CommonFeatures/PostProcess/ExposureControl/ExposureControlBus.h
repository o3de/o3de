/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>

namespace AZ
{
    namespace Render
    {
        class ExposureControlRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::ExposureControlRequests, "{2FAF1C0B-9306-4ECA-971B-1A17F9170F5B}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~ExposureControlRequests() {}

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<ExposureControlRequests> ExposureControlRequestBus;
    }
}
