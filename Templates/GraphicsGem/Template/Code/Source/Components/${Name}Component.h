/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/${Name}ComponentController.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace ${Name}
{
    inline constexpr AZ::TypeId ${Name}ComponentTypeId { "{${Random_Uuid}}" };

    class ${Name}Component final
        : public AzFramework::Components::ComponentAdapter<${Name}ComponentController, ${Name}ComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<${Name}ComponentController, ${Name}ComponentConfig>;
        AZ_COMPONENT(${Name}Component, ${Name}ComponentTypeId, BaseClass);

        ${Name}Component() = default;
        ${Name}Component(const ${Name}ComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
}
