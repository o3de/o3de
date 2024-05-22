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
#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    namespace Render
    {
        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(
            FogMode,
            uint8_t,
            (Linear, 0),            //!< f = (end - d)/(end - start). "d" represents depth, or the distance from the viewpoint
            (Exponential, 1),       //!< f = 1/exp(d * density). "density" is an arbitrary fog density that can range from 0.0 to 1.0.
            (ExponentialSquared, 2) //!< f = 1/exp((d * density)^2). "density" is an arbitrary fog density that can range from 0.0 to 1.0.
        );

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
