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

namespace AZ
{
    namespace Render
    {
        class DeferredFogSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::DeferredFogSettingsInterface, "{4C760B7D-DCBB-4244-94E4-E17180CA722A}");

            virtual void OnSettingsChanged() = 0;

            virtual void SetInitialized(bool isInitialized) = 0;
            virtual bool IsInitialized() = 0;

            virtual void SetEnabled(bool value) = 0;
            virtual bool GetEnabled() const = 0;

            virtual void SetUseNoiseTextureShaderOption(bool value)= 0;
            virtual bool GetUseNoiseTextureShaderOption()= 0;
            virtual void SetEnableFogLayerShaderOption(bool value)= 0;
            virtual bool GetEnableFogLayerShaderOption()= 0;

            // Auto-gen virtual getter and setter functions...
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        virtual ValueType Get##Name() const = 0;                                                        \
        virtual void Set##Name(ValueType val) = 0;                                                      \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

    }
}
