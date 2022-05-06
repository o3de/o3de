/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialConfiguration.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    //! Runtime PhysX material, created from a MaterialConfiguration.
    // TODO: Material2 is temporary until old Material class is removed.
    class Material2
        : public Physics::Material2
    {
    public:
        Material2(const Physics::MaterialConfiguration2& configuration);

        // Physics::Material2 overrides ...
        float GetDynamicFriction() const override;
        void SetDynamicFriction(float dynamicFriction) override;
        float GetStaticFriction() const override;
        void SetStaticFriction(float staticFriction) override;
        float GetRestitution() const override;
        void SetRestitution(float restitution) override;
        Physics::CombineMode GetFrictionCombineMode() const override;
        void SetFrictionCombineMode(Physics::CombineMode mode) override;
        Physics::CombineMode GetRestitutionCombineMode() const override;
        void SetRestitutionCombineMode(Physics::CombineMode mode) override;
        float GetDensity() const override;
        void SetDensity(float density) override;
        const AZ::Color& GetDebugColor() const override;
        void SetDebugColor(const AZ::Color& debugColor) override;
        const void* GetNativePointer() const override;

    private:
        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        float m_density = 1000.0f;
        AZ::Color m_debugColor = AZ::Colors::White;
    };
} // namespace PhysX
