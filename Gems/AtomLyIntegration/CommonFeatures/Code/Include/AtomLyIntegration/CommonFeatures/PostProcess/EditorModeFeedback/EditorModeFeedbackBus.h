/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        class EditorModeFeedbackRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeedbackRequests, "{ADF4284F-5CA3-4EA6-BC7A-F434CBFCB179}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~EditorModeFeedbackRequests() {}
            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<EditorModeFeedbackRequests> EditorModeFeedbackRequestBus;
    } // namespace Render
} // namespace AZ
