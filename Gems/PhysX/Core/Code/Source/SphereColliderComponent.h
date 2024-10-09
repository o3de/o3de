
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BaseColliderComponent.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

namespace PhysX
{
    /// Component that provides sphere shape collider.
    /// May be used in conjunction with a PhysX Rigid Body Component to create a dynamic rigid body, or on its own
    /// to create a static rigid body.
    class SphereColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::SphereShapeConfiguration;
        AZ_COMPONENT(SphereColliderComponent, "{108CD341-E5C3-4AE1-B712-21E81ED6C277}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        SphereColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
