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

namespace PhysX
{
    /// Component that provides capsule shape collider.
    /// May be used in conjunction with a PhysX Rigid Body Component to create a dynamic rigid body, or on its own
    /// to create a static rigid body.
    class CapsuleColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::CapsuleShapeConfiguration;
        AZ_COMPONENT(CapsuleColliderComponent, "{E0622390-4E66-43D5-8F2F-5A928C9D1DCC}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        CapsuleColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
