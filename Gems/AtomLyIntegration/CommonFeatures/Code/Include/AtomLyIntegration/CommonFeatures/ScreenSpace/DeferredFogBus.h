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

namespace AZ
{
    namespace Render
    {
        class DeferredFogRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::DeferredFogRequests, "{B60F03EB-36D3-426B-91DE-4FB7B5EAE6CD}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~DeferredFogRequests() {}

            // Auto-gen virtual getter and setter functions - matching the interface methods    
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const = 0;                                                        \
        virtual void Set##Name(ValueType val) = 0;                                                      \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<DeferredFogRequests> DeferredFogRequestsBus;
    }
}
