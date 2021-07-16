/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Parameter;

    class ParameterFactory
    {
    public:
        static void ReflectParameterTypes(AZ::ReflectContext* context);

        static AZStd::vector<AZ::TypeId> GetValueParameterTypes();
        static AZStd::vector<AZ::TypeId> GetParameterTypes();

        static Parameter* Create(const AZ::TypeId& type);
    };
}
