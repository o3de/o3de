/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/GraphicsGem_AR_TestComponentController.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace GraphicsGem_AR_Test
{
    inline constexpr AZ::TypeId GraphicsGem_AR_TestComponentTypeId { "{FD611FC2-B6E7-47F9-BBBC-1A0EE9D6E345}" };

    class GraphicsGem_AR_TestComponent final
        : public AzFramework::Components::ComponentAdapter<GraphicsGem_AR_TestComponentController, GraphicsGem_AR_TestComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<GraphicsGem_AR_TestComponentController, GraphicsGem_AR_TestComponentConfig>;
        AZ_COMPONENT(GraphicsGem_AR_TestComponent, GraphicsGem_AR_TestComponentTypeId, BaseClass);

        GraphicsGem_AR_TestComponent() = default;
        GraphicsGem_AR_TestComponent(const GraphicsGem_AR_TestComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
}