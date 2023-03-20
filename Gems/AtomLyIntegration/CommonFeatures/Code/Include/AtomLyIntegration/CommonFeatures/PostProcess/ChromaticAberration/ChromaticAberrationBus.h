/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class ChromaticAberrationRequests : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::ChromaticAberrationRequests, "{400844DF-0B44-4941-AECD-135D94909E16}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~ChromaticAberrationRequests()
            {
            }

            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<ChromaticAberrationRequests> ChromaticAberrationRequestBus;
    } // namespace Render
} // namespace AZ
