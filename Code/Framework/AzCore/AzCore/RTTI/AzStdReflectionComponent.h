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