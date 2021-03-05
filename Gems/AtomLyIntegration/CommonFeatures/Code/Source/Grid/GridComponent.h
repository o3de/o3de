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
