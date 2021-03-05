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

// Auto-generates edit context reflection code for the specified overrides
//
// Note: To use this you must first define EDITOR_CLASS as the class you are serializing
// Example: #define EDITOR_CLASS DofComponentConfig

#define AZ_GFX_ANY_PARAM_BOOL_OVERRIDE(ValueType, Name, MemberName)                                                                         \
        ->DataElement(Edit::UIHandlers::CheckBox, &EDITOR_CLASS::MemberName##Override, #Name " Override", "Override enable for " #Name)     \
            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)                                            \

#define AZ_GFX_INTEGER_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                                                    \
        ->DataElement(Edit::UIHandlers::Slider, &EDITOR_CLASS::MemberName##Override, #Name " Override", "Override factor for " #Name)       \
            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)                                            \
            ->Attribute(Edit::Attributes::Min, 0.0f)                                                                                        \
            ->Attribute(Edit::Attributes::Max, 1.0f)                                                                                        \

#define AZ_GFX_FLOAT_PARAM_FLOAT_OVERRIDE(ValueType, Name, MemberName)                                                                      \
        ->DataElement(Edit::UIHandlers::Slider, &EDITOR_CLASS::MemberName##Override, #Name " Override", "Override factor for " #Name)       \
            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)                                            \
            ->Attribute(Edit::Attributes::Min, 0.0f)                                                                                        \
            ->Attribute(Edit::Attributes::Max, 1.0f)                                                                                        \

#include <Atom/Feature/ParamMacros/MapParamEmpty.inl>
