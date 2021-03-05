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

#include <ReflectionProbe/ReflectionProbeComponentController.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeComponent final
            : public AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>;
            AZ_COMPONENT(AZ::Render::ReflectionProbeComponent, ReflectionProbeComponentTypeId, BaseClass);

            ReflectionProbeComponent() = default;
            ReflectionProbeComponent(const ReflectionProbeComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
