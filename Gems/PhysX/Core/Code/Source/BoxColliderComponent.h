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
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace PhysX
{
    /// Component that provides box shape collider.
    /// May be used in conjunction with a PhysX Rigid Body Component to create a dynamic rigid body, or on its own
    /// to create a static rigid body.
    class BoxColliderComponent
        : public BaseColliderComponent
    {
    public:
        using Configuration = Physics::BoxShapeConfiguration;
        AZ_COMPONENT(BoxColliderComponent, "{881D85FC-7D85-4E4F-B58C-80BD4C94A51A}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        BoxColliderComponent() = default;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
}
