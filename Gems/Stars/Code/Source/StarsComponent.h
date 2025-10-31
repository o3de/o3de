/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <StarsComponentController.h>

namespace AZ::Render 
{
    class StarsComponent final
        : public AzFramework::Components::ComponentAdapter<StarsComponentController, StarsComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<StarsComponentController, StarsComponentConfig>;
        AZ_COMPONENT(AZ::Render::StarsComponent, StarsComponentTypeId , BaseClass);

        StarsComponent() = default;
        StarsComponent(const StarsComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AZ::Render 
