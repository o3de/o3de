
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
