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
