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

#include <Mesh/MeshComponentController.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>


namespace AZ
{
    namespace Render
    {
        class MeshComponent final
            : public AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig, true>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig, true>;
            AZ_COMPONENT(AZ::Render::MeshComponent, MeshComponentTypeId, BaseClass);

            MeshComponent() = default;
            MeshComponent(const MeshComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
