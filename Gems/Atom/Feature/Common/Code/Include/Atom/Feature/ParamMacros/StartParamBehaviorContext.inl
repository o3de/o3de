/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Auto-generates behavior context reflection code for the specified parameters and overrides
//
// Note: To use this you must first define PARAM_EVENT_BUS as the events bus you are using for the behavior context
// Example: #define PARAM_EVENT_BUS DepthOfFieldRequestBus::Events

#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ->Event("Set" #Name, &PARAM_EVENT_BUS::Set##Name)                                               \
        ->Event("Get" #Name, &PARAM_EVENT_BUS::Get##Name)                                               \
        ->VirtualProperty(#Name, "Get" #Name, "Set" #Name)                                              \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        ->Event("Set" #Name "Override", &PARAM_EVENT_BUS::Set##Name##Override)                          \
        ->Event("Get" #Name "Override", &PARAM_EVENT_BUS::Get##Name##Override)                          \
        ->VirtualProperty(#Name "Override", "Get" #Name "Override", "Set" #Name "Override")             \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
