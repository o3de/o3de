/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h>

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
