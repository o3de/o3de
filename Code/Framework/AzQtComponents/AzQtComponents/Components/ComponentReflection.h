/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptTypeProvider.h>

#include <AzQtComponents/AzQtComponentsAPI.h>

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ComponentReflection
    {
    public:
        static void Reflect(AZ::ReflectContext* context, const AZ::EnvironmentInstance& env);
    };
} // namespace AzQtComponents
