/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
