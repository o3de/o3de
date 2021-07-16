/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            : public AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig>;
            AZ_COMPONENT(AZ::Render::MeshComponent, MeshComponentTypeId, BaseClass);

            MeshComponent() = default;
            explicit MeshComponent(const MeshComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
