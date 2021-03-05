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

#include "FloatParameter.h"

namespace EMotionFX
{
    class FloatSliderParameter
        : public FloatParameter
    {
    public:
        AZ_RTTI(FloatSliderParameter, "{2ED6BBAF-5C82-4EAA-8678-B220667254F2}", FloatParameter)
        AZ_CLASS_ALLOCATOR_DECL

        FloatSliderParameter()
            : FloatParameter()
        {}

        explicit FloatSliderParameter(AZStd::string name, AZStd::string description = {})
            : FloatParameter(AZStd::move(name), AZStd::move(description))
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeDisplayName() const override;
    };
} // namespace EMotionFX
