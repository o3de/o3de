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
#include <AzFramework/Components/ComponentAdapter.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentConfig.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentConstants.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentController.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseGlobalIlluminationComponent final
            : public AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>;
            AZ_COMPONENT(AZ::Render::DiffuseGlobalIlluminationComponent, DiffuseGlobalIlluminationComponentTypeId , BaseClass);

            DiffuseGlobalIlluminationComponent() = default;
            DiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
