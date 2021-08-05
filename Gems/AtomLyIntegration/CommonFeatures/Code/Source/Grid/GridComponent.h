/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Grid/GridComponentController.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class GridComponent final
            : public AzFramework::Components::ComponentAdapter<GridComponentController, GridComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<GridComponentController, GridComponentConfig>;
            AZ_COMPONENT(AZ::Render::GridComponent, GridComponentTypeId, BaseClass);

            GridComponent() = default;
            GridComponent(const GridComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
