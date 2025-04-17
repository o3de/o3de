/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceConstants.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class WhiteBalanceRequests : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::WhiteBalanceRequests, "{B5F44660-F39C-476F-AB27-344FA76C3715}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~WhiteBalanceRequests()
            {
            }

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<WhiteBalanceRequests> WhiteBalanceRequestBus;
    } // namespace Render
} // namespace AZ
