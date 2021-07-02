/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ
{
    class AzStdReflectionComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AzStdReflectionComponent, "{E6049565-B346-4F54-B9A5-FC7354384ACB}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        ~AzStdReflectionComponent() override = default;
        void Activate() override { }
        void Deactivate() override { }
    };
}
