/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/ACES/Aces.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace Render
    {
        class LookModificationRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::LookModificationRequests, "{F1E49309-8963-4F96-9EF8-BFC791E0DC36}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~LookModificationRequests() {}
            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<LookModificationRequests> LookModificationRequestBus;
    }
}
