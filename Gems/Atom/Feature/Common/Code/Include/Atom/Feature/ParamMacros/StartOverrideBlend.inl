/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates code to override target's params with our own using provided override values
//
// Note: To use this you must first #define OVERRIDE_TARGET and OVERRIDE_ALPHA
// OVERRIDE_TARGET is a pointer to the settings you are overriding
// OVERRIDE_ALPHA is a blend factor that modulates the member blend
//
// Example: (see DepthOfFieldSettings::ApplySettingsTo)
// #define OVERRIDE_TARGET target
// #define OVERRIDE_ALPHA alpha

#define AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(ValueType, Name, MemberName)                                     \
        {                                                                                               \
            bool applyOverride = MemberName##Override && (OVERRIDE_ALPHA >= 0.5f);                      \
            if(applyOverride)                                                                           \
            {                                                                                           \
                OVERRIDE_TARGET->Set##Name(MemberName);                                                 \
            }                                                                                           \
        }                                                                                               \

#define AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                \
        {                                                                                               \
            float alphaFactorThis = OVERRIDE_ALPHA * MemberName##Override;                              \
            float alphaFactorTarget = 1.0f - alphaFactorThis;                                           \
            ValueType newValue = ValueType(this->MemberName * alphaFactorThis);                         \
            newValue = newValue + ValueType(OVERRIDE_TARGET->MemberName * alphaFactorTarget);           \
            OVERRIDE_TARGET->Set##Name(newValue);                                                       \
        }                                                                                               \

#define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                  \
        {                                                                                               \
            float alphaFactorThis = OVERRIDE_ALPHA * MemberName##Override;                              \
            float alphaFactorTarget = 1.0f - alphaFactorThis;                                           \
            ValueType newValue = ValueType(this->MemberName * alphaFactorThis);                         \
            newValue = newValue + ValueType(OVERRIDE_TARGET->MemberName * alphaFactorTarget);           \
            OVERRIDE_TARGET->Set##Name(newValue);                                                       \
        }                                                                                               \

#include <Atom/Feature/ParamMacros/MapParamEmpty.inl>
